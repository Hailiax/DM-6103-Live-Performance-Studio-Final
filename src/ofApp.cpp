#include "ofApp.h"
#include "FastNoiseSIMD.h"

//--------------------------------------------------------------
void ofApp::setup(){
    particleSize = 0.5f;
    particleLife = 1.8f;
    timeStep = 0.005f;
    numParticles = 1000000;
    dancerRadiusSquared = 50*50;
    fractalScale = 0.006;
    
    // Seting the textures where the information ( position and velocity ) will be
    textureRes = (int)sqrt((float)numParticles);
    numParticles = textureRes * textureRes;
    
    // Width and Heigth of the windows
    width = ofGetWindowWidth();
    height = ofGetWindowHeight();
    
    // Loading the Shaders
    updatePos.load("passthru.vert", "posUpdate.frag");// shader for updating the texture that store the particles position on RG channels
    updateVel.load("passthru.vert", "velUpdate.frag");// shader for updating the texture that store the particles velocity on RG channels
    updateAge.load("passthru.vert", "ageUpdate.frag");// shader for updating the texture that store the particles velocity on R channel
    
    // Frag, Vert and Geo shaders for the rendering process of the spark image
    updateRender.setGeometryInputType(GL_POINTS);
    updateRender.setGeometryOutputType(GL_TRIANGLE_STRIP);
    updateRender.setGeometryOutputCount(6);
    updateRender.load("render.vert","render.frag","render.geom");
    
    // 1. Making arrays of float pixels with position information
    vector<float> pos(numParticles*3);
    for (int x = 0; x < textureRes; x++){
        for (int y = 0; y < textureRes; y++){
            int i = textureRes * y + x;
            
            pos[i*3 + 0] = ofRandom(1.0); //x*offset;
            pos[i*3 + 1] = ofRandom(1.0); //y*offset;
            pos[i*3 + 2] = 0.0f;
        }
    }
    // Load this information in to the FBO's texture
    posPingPong.allocate(textureRes, textureRes, GL_RGB32F);
    origPos.allocate(textureRes, textureRes, GL_RGB32F);
    origPos.getTexture().loadData(pos.data(), textureRes, textureRes, GL_RGB);
    posPingPong.src->getTexture().loadData(pos.data(), textureRes, textureRes, GL_RGB);
    
    // 2. Making arrays of float pixels with velocity information and the load it to a texture
    vector<float> vel(numParticles*3);
    for (int i = 0; i < numParticles; i++){
        vel[i*3 + 0] = ofRandom(-1.0,1.0);
        vel[i*3 + 1] = ofRandom(-1.0,1.0);
        vel[i*3 + 2] = 1.0f;
    }
    // Load this information in to the FBO's texture
    velPingPong.allocate(textureRes, textureRes, GL_RGB32F);
    origVel.allocate(textureRes, textureRes, GL_RGB32F);
    origVel.getTexture().loadData(vel.data(), textureRes, textureRes, GL_RGB);
    velPingPong.src->getTexture().loadData(vel.data(), textureRes, textureRes, GL_RGB);
    
    // 3. Making arrays of float pixels with age information and the load it to a texture
    vector<float> age(numParticles*3);
    for (int i = 0; i < numParticles; i++){
        age[i*3 + 0] = 1.0*i/numParticles*(particleLife+timeStep) - timeStep;
        age[i*3 + 1] = 1.0;
        age[i*3 + 2] = 1.0;
    }
    // Load this information in to the FBO's texture
    agePingPong.allocate(textureRes, textureRes, GL_RGB32F);
    agePingPong.src->getTexture().loadData(age.data(), textureRes, textureRes, GL_RGB);
    
    // Loading and setings of the variables of the textures of the particles
    particleImg.load("dot.png");
    
    // Allocate the final
    renderFBO.allocate(width, height, GL_RGB32F);
    renderFBO.begin();
    ofClear(0, 0, 0, 255);
    renderFBO.end();
    
    mesh.setMode(OF_PRIMITIVE_POINTS);
    for(int x = 0; x < textureRes; x++){
        for(int y = 0; y < textureRes; y++){
            mesh.addVertex(ofVec3f(x,y));
            mesh.addTexCoord(ofVec2f(x, y));
        }
    }
    
    // Create fractal noise map array values -1 : 1
    int fractalRes = width;
    if (height > width) fractalRes = height;
    
    vector<float> fractalVec(fractalRes*fractalRes*3);
    
    FastNoiseSIMD* fractalNoise1 = FastNoiseSIMD::NewFastNoiseSIMD();
    fractalNoise1->SetFrequency(fractalScale);
    fractalNoise1->SetSeed(1000);
    float* noiseSet1 = fractalNoise1->GetSimplexFractalSet(0, 0, 0, fractalRes, fractalRes, 1);
    
    FastNoiseSIMD* fractalNoise2 = FastNoiseSIMD::NewFastNoiseSIMD();
    fractalNoise2->SetFrequency(fractalScale);
    fractalNoise2->SetSeed(2000);
    float* noiseSet2 = fractalNoise2->GetSimplexFractalSet(0, 0, 0, fractalRes, fractalRes, 1);
    
    for (int i = 0; i < fractalRes*fractalRes; i++){
        fractalVec[i*3 + 0] = noiseSet1[i];
        fractalVec[i*3 + 1] = noiseSet2[i];
        fractalVec[i*3 + 2] = 0.0;
    }
    
    FastNoiseSIMD::FreeNoiseSet(noiseSet1);
    FastNoiseSIMD::FreeNoiseSet(noiseSet2);
    
    fractal.allocate(fractalRes, fractalRes, GL_RGB32F);
    fractal.getTexture().loadData(fractalVec.data(), fractalRes, fractalRes, GL_RGB);
}

//--------------------------------------------------------------
void ofApp::update(){
    
    // Ages PingPong
    agePingPong.dst->begin();
    ofClear(0);
    updateAge.begin();
    updateAge.setUniformTexture("prevAgeData", agePingPong.src->getTexture(), 0);   // passing the previus age information
    updateAge.setUniform1f("timestep", (float)timeStep);
    updateAge.setUniform1f("life", particleLife);
    
    // draw the source age texture to be updated
    agePingPong.src->draw(0, 0);
    
    updateAge.end();
    agePingPong.dst->end();
    
    agePingPong.swap();
    
    
    // Velocities PingPong
    velPingPong.dst->begin();
    ofClear(0);
    updateVel.begin();
    updateVel.setUniformTexture("backbuffer", velPingPong.src->getTexture(), 0);   // passing the previus velocity information
    updateVel.setUniformTexture("origVelData", origVel.getTexture(), 1);  // passing the position information
    updateVel.setUniformTexture("posData", posPingPong.src->getTexture(), 2);  // passing the position information
    updateVel.setUniformTexture("ageData", agePingPong.src->getTexture(), 3);  // passing the position information
    updateVel.setUniform2f("screen", (float)width, (float)height);
    updateVel.setUniform2f("mouse", (float)mouseX, (float)mouseY);
    updateVel.setUniform1f("dancerRadiusSquared", (float)dancerRadiusSquared);
    updateVel.setUniform1f("timestep", (float)timeStep);
    
    // draw the source velocity texture to be updated
    velPingPong.src->draw(0, 0);
    
    updateVel.end();
    velPingPong.dst->end();
    
    velPingPong.swap();
    
    
    // Positions PingPong
    posPingPong.dst->begin();
    ofClear(0);
    updatePos.begin();
    updatePos.setUniformTexture("prevPosData", posPingPong.src->getTexture(), 0); // Previus position
    updatePos.setUniformTexture("origPosData", origPos.getTexture(), 1);  // passing the position information
    updatePos.setUniformTexture("velData", velPingPong.src->getTexture(), 2);  // Velocity
    updatePos.setUniformTexture("ageData", agePingPong.src->getTexture(), 3);  // Velocity
    updatePos.setUniformTexture("fractalData", fractal.getTexture(), 4);  // Velocity
    updatePos.setUniform2f("screen", (float)width, (float)height);
    updatePos.setUniform1f("timestep", (float) timeStep);
    
    // draw the source position texture to be updated
    posPingPong.src->draw(0, 0);
    
    updatePos.end();
    posPingPong.dst->end();
    
    posPingPong.swap();
    
    
    // Rendering
    //
    // 1.   Sending this vertex to the Vertex Shader.
    //      Each one it's draw exactly on the position that match where it's stored on both vel and pos textures
    //      So on the Vertex Shader (that's is first at the pipeline) can search for it information and move it
    //      to it right position.
    // 2.   Once it's in the right place the Geometry Shader make 6 more vertex in order to make a billboard
    // 3.   that then on the Fragment Shader is going to be filled with the pixels of sparkImg texture
    //
    renderFBO.begin();
    ofClear(0,0,0,0);
    updateRender.begin();
    updateRender.setUniformTexture("posTex", posPingPong.dst->getTexture(), 0);
    updateRender.setUniformTexture("imgTex", particleImg.getTexture() , 1);
    updateRender.setUniform1i("resolution", (float)textureRes);
    updateRender.setUniform2f("screen", (float)width, (float)height);
    updateRender.setUniform1f("particleSize", (float)particleSize);
    updateRender.setUniform1f("imgWidth", (float)particleImg.getWidth());
    updateRender.setUniform1f("imgHeight", (float)particleImg.getHeight());
    
    ofPushStyle();
    ofEnableBlendMode( OF_BLENDMODE_ADD );
    ofSetColor(255);
    
    mesh.draw();
    
    ofDisableBlendMode();
    glEnd();
    
    updateRender.end();
    renderFBO.end();
    ofPopStyle();
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(0);
    
    ofSetColor(255);
    renderFBO.draw(0,0);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    
}

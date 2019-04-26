#include "ofApp.h"

//--------------------------------------------------------------

//
// Note: there are computer specific settings for: OpenGL 3, SIMD 4.1, and Apple Clang
//

void ofApp::setup(){
    particleSize = 1.5f;
    particleLife = 15.0f;
    velocityScale = 0.7f;
    opposingVelocity = -0.5f; // Cool as -15.0
    timeStep = 0.005f;
    numParticles = 500000; // Turn up as much as possible
    dancerRadiusSquared = 50*50;
    frameWidth = 25;
    phase = 1;
    
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
    densityFilter.load("passthru.vert", "densityFilter.frag");
    blurX.load("passthru.vert", "blurX.frag");
    blurY.load("passthru.vert", "blurY.frag");
    glowAdd.load("passthru.vert", "glowAdd.frag");
    
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
    
    // Allocate the effects
    effectsPingPong.allocate(width, height, GL_RGB32F);
    glowAddFBO.allocate(width, height, GL_RGB32F);
    
    // Create the frame mesh
    
    
    // Create fractal noise map array values -1 : 1
    fractalRes = width;
    if (height > width) fractalRes = height;
    fractal.allocate(fractalRes, fractalRes, GL_RGB32F);
    fractalGenerator.setup(fractalRes); // Fractal is transferred to texture at start of update()
}

//--------------------------------------------------------------
void ofApp::update(){
    // Create new fractal on different thread
    if (!fractalGenerator.isThreadRunning()){
        fractal.getTexture().loadData(fractalGenerator.fractalVec->data(), fractalRes, fractalRes, GL_RGB);
        fractalGenerator.startThread();
    }
    
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
    updateVel.setUniform1f("opposingVelocity", (float)opposingVelocity);
    
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
    updatePos.setUniformTexture("ageData", agePingPong.src->getTexture(), 3);  // Age
    updatePos.setUniformTexture("fractalData", fractal.getTexture(), 4);  // Fractal
    updatePos.setUniform2f("screen", (float)width, (float)height);
    updatePos.setUniform1f("timestep", (float) timeStep);
    updatePos.setUniform1f("velocityScale", (float)velocityScale);
    updatePos.setUniform1i("phase", (int)phase);
    
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
    
    // Transfer renderFBO to effectsPingPong src
    effectsPingPong.src->begin();
    ofClear(0);
    renderFBO.draw(0,0);
    effectsPingPong.src->end();
    
    // Apply effects
    effectsPingPong.dst->begin(); // Density effect
    ofClear(0);
    densityFilter.begin();
    densityFilter.setUniformTexture("fractalData", fractal.getTexture(), 1);
    densityFilter.setUniform1f("frameNum", (float)sin(ofGetFrameNum()));
    effectsPingPong.src->draw(0,0);
    densityFilter.end();
    effectsPingPong.dst->end();
    effectsPingPong.swap();

    glowAddFBO.begin(); // Hold current frame in FBO for glow effect
    ofClear(0);
    effectsPingPong.src->draw(0,0);
    glowAddFBO.end();

    effectsPingPong.dst->begin(); // Blur X
    ofClear(0);
    blurX.begin();
    blurX.setUniform1f("blurAmount", 8.0);
    effectsPingPong.src->draw(0,0);
    blurX.end();
    effectsPingPong.dst->end();
    effectsPingPong.swap();

    effectsPingPong.dst->begin(); // Blur Y
    ofClear(0);
    blurY.begin();
    blurY.setUniform1f("blurAmount", 8.0);
    effectsPingPong.src->draw(0,0);
    blurY.end();
    effectsPingPong.dst->end();
    effectsPingPong.swap();

    effectsPingPong.dst->begin(); // Combine blur and stored renderFBO for glow
    ofClear(0);
    glowAdd.begin();
    glowAdd.setUniformTexture("sharpTex", glowAddFBO.getTexture() , 1);
    glowAdd.setUniform2f("screen", (float)width, (float)height);
    glowAdd.setUniform1i("frameWidth", frameWidth);
    effectsPingPong.src->draw(0,0);
    glowAdd.end();
    effectsPingPong.dst->end();
    effectsPingPong.swap();
    
    
    
    // Live Effects
    dancerRadiusSquared = sin(ofGetFrameNum()*0.1)*20 + 40;
    dancerRadiusSquared *= dancerRadiusSquared;
    frameWidth = sin(ofGetFrameNum()*0.1)*5 + 25;
    
    if (effectWindExplode){
        unsigned int f = ofGetFrameNum() - effectWindExplode;
        if (f == 0){
            phase = 2;
            opposingVelocity = -30.0;
        } else if (f == 45){
            phase = 1;
            opposingVelocity = -0.5;
            effectWindExplode = 0;
        }
    }
    if (effectQuickExplode){
        unsigned int f = ofGetFrameNum() - effectQuickExplode;
        if (f == 0){
            opposingVelocity = -15.0;
        } else if (f == 5){
            opposingVelocity = -0.5;
            effectQuickExplode = 0;
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(0);
    
    ofSetColor(255);
    effectsPingPong.src->draw(0,0);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    // Phases
    if (key == '1'){
        phase = 1;
//        particleLife = 15.0f;
    } else if (key == '2'){
        phase = 2;
//        particleLife = 3.0f;
    }
    
    // Effects
    else if (key == 'q')
        effectWindExplode = ofGetFrameNum();
    else if (key == 'w')
        effectQuickExplode = ofGetFrameNum();
    
    // Modifiers
    else if (key == 'z'){
        opposingVelocity = -0.5;
    } else if (key == 'x'){
        opposingVelocity = -15.0;
    } else if (key == 'c'){
        velocityScale = 0.7f;
    } else if (key == 'v'){
        velocityScale = -0.7f;
    }
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


#include "ofSecond.h"

//--------------------------------------------------------------

void ofSecond::setup(){
    
}

//--------------------------------------------------------------
void ofSecond::update(){
    
}

//--------------------------------------------------------------
void ofSecond::draw(){
    ofBackground(0);
    
    main->effectsPingPong.src->getTexture().drawSubsection(0,0,main->width,main->height/2,0,0);
    
    main->frameMesh.draw();
}

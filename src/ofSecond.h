#pragma once

#include "ofApp.h"

class ofSecond : public ofBaseApp{
public:
    void setup();
    void update();
    void draw();
    
    shared_ptr<ofApp> main;
};

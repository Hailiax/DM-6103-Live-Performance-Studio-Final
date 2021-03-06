#include "ofMain.h"
#include "ofAppGLFWWindow.h"
#include "ofApp.h"
#include "ofSecond.h"

//========================================================================
int main(){
    
    // Change to true if using multiple full screen projections
    bool production = false;
    
    
    ofGLFWWindowSettings settings;
    settings.setGLVersion(3, 2);
    settings.resizable = false;
    settings.decorated = false;
    
    if (production) settings.windowMode = OF_FULLSCREEN;
    else settings.setSize(896, 504);
    
    if (production) settings.monitor = 1;
    else settings.setPosition(ofVec2f(0,504));
    
    shared_ptr<ofAppBaseWindow> mainWindow = ofCreateWindow(settings);
    
    if (production) settings.monitor = 2;
    else settings.setPosition(ofVec2f(0,0));
    
    settings.shareContextWith = mainWindow;
    shared_ptr<ofAppBaseWindow> secondWindow = ofCreateWindow(settings);
    
    shared_ptr<ofApp> mainApp(new ofApp);
    shared_ptr<ofSecond> secondApp(new ofSecond);
    secondApp->main = mainApp;
    
    ofRunApp(mainWindow, mainApp);
    ofRunApp(secondWindow, secondApp);
    ofRunMainLoop();
    
}

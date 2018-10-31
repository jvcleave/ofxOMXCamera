/*
 *  ofxRPiCameraPhotoGrabber.h
 *
 *  Created by jason van cleave on 6/1/13.
 *
 */

#pragma once

#include "ofMain.h"
#include "ofAppEGLWindow.h"

#include "RPICameraController.h"

#include "PhotoEngine.h"





class ofxRPiCameraPhotoGrabber : public RPICameraController, public PhotoEngineListener
{

public:
    
	ofxRPiCameraPhotoGrabber();
    ~ofxRPiCameraPhotoGrabber();
    void setup(OMXCameraSettings&, bool doApplySettings=false);
    bool isReady();
	int getWidth();
	int getHeight();
    void takePhoto();
    PhotoEngine* engine;
    
    
    void draw(ofRectangle& rectangle);
    void draw(int x, int y, int width, int height);
    void setDisplayAlpha(int);
    void setDisplayLayer(int);
    void setDisplayRotation(int);
    void setDisplayDrawRectangle(ofRectangle);
    void setDisplayCropRectangle(ofRectangle);
    void setDisplayMirror(bool);

    ofxRPiCameraPhotoGrabberListener* listener;
    
    void onTakePhotoComplete(string filePath) override;
    vector<string> photosTaken;
};

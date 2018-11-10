/*
 *  ofxOMXPhotoGrabber.h
 *
 *  Created by jason van cleave on 6/1/13.
 *
 */

#pragma once

#include "ofMain.h"
#include "ofAppEGLWindow.h"

#include "OMXCameraController.h"

#include "PhotoEngine.h"





class ofxOMXPhotoGrabber : public OMXCameraController, public PhotoEngineListener
{

public:
    
	ofxOMXPhotoGrabber();
    ~ofxOMXPhotoGrabber();
    
    //OMXCameraController
    void setup(ofxOMXCameraSettings&) override;
    void close() override;
    bool isReady() override;

    void draw() override;
    void draw(int x, int y) override;
    void draw(int x, int y, int width, int height) override;
    void draw(ofRectangle&) override;
    int getWidth() override;
    int getHeight() override;

    ofxOMXPhotoGrabberListener* listener;

    //PhotoEngineListener
    void onTakePhotoComplete(string filePath) override;
    void onPhotoEngineStart(OMX_HANDLETYPE camera_) override;
    
    PhotoEngine engine;
    vector<string> photosTaken;
    void takePhoto(int numShots=0);
    void setJPEGCompression(int); // 1-100
};

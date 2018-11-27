/*
 *  ofxOMXVideoGrabber.h
 *
 *  Created by jason van cleave on 6/1/13.
 *
 */

#pragma once

#include "OMXCameraController.h"
#include "ofAppEGLWindow.h"
#include "VideoEngine.h"

using namespace std;



class ofxOMXVideoGrabber : public OMXCameraController, public VideoEngineListener
{

public:

	ofxOMXVideoGrabber();
    ~ofxOMXVideoGrabber();
    
    //OMXCameraController
    void setup(ofxOMXCameraSettings&)override;
    void close() override;
    bool isReady() override;
    
    void draw() override;
    void draw(int x, int y)override;
    void draw(int x, int y, int width, int height)override;
    void draw(ofRectangle&)override;
    int getWidth()override;
    int getHeight()override;


    int getFrameRate();
    bool isFrameNew();

    bool isRecording();
    void startRecording();
    void stopRecording();
    
 
    //VideoEngineListener
    VideoEngine engine;
    void onRecordingComplete(string filePath) override;
    void onVideoEngineStart() override;
    void onVideoEngineClose() override;
    
private:
    bool hasNewFrame;
    int updateFrameCounter;
    int frameCounter;
	void onUpdate(ofEventArgs & args);
    
};

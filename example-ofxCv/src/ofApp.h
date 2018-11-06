#pragma once

#include "ofMain.h"
#include "TerminalListener.h"
#include "ofxOMXVideoGrabber.h"
#include "ofxCv.h"


class ofApp : public ofBaseApp, public KeyListener{

	public:

		void setup();
		void update();
		void draw();
		void keyPressed(int key);

	void onCharacterReceived(KeyListenerEventData& e)
    {
        keyPressed((int)e.character);
    };

	TerminalListener consoleListener;
    
    ofxOMXVideoGrabber videoGrabber;
    ofxOMXCameraSettings settings;
    
    
    cv::Mat accumulatorMat;
    cv::Mat frameMat;
    cv::Mat backgroundOutputMat;
    ofImage backgroundOutputImage;


    bool doEraseBackground;
    float alpha;
    float threshold;
};


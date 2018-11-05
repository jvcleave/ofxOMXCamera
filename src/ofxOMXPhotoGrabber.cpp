#include "ofxOMXPhotoGrabber.h"

int shotsRequested;
int shotsTaken;

ofxOMXPhotoGrabber::ofxOMXPhotoGrabber()
{
    camera = NULL;
    shotsRequested = 0;
    shotsTaken = 0;
}

int totalTime = 0;

void ofxOMXPhotoGrabber::setup(ofxOMXCameraSettings settings_)
{
    settings = settings_;
    listener = settings.photoGrabberListener;
    
    
    
    
    ofLogNotice(__func__) << settings.toString();
   
    if(settings.enableTexture)
    {
        displayController.generateEGLImage(settings.stillPreviewWidth, settings.stillPreviewHeight);
        engine.setup(&settings, this, &displayController);  

    }else
    {
        engine.setup(&settings, this, NULL);  

    }

}


void ofxOMXPhotoGrabber::onPhotoEngineStart(OMX_HANDLETYPE camera_)
{
    camera = camera_;
    
    applyAllSettings();
    ofLogNotice(__func__) << "shotsRequested: " << shotsRequested << " shotsTaken: " << shotsTaken;
    if(shotsTaken)
    {
        //setBurstMode(settings.burstModeEnabled);
        if(shotsTaken >= 1)
        {
            //arbitrary timing so exposure doesn't fade to black :/
            ofSleepMillis(300);
        }
        if(shotsTaken < shotsRequested)
        {
            takePhoto();
        }else
        {
            ofLog() << shotsTaken << " TOOK totalTime: " << totalTime << " EACH " << totalTime/shotsTaken;
        }
    }
}


bool ofxOMXPhotoGrabber::isReady()
{
    return engine.isOpen();
}


int photoStart = 0;
void ofxOMXPhotoGrabber::takePhoto(int numShots)
{   
    shotsRequested+=numShots;
    if(camera)
    {
        photoStart = ofGetElapsedTimeMillis();
        engine.takePhoto();
    }
    
    camera = NULL;
}

void ofxOMXPhotoGrabber::onTakePhotoComplete(string filePath)
{
    
    int end = ofGetElapsedTimeMillis();
    int total = end-photoStart;
    ofLog() << "PHOTO TOOK: " <<  total << "MS";
    totalTime +=total;
    photoStart = 0;
    
    photosTaken.push_back(filePath);
    shotsTaken++;
    ofLogNotice(__func__) << "shotsRequested: " << shotsRequested << " shotsTaken: " << shotsTaken;

    if(listener)
    {
        listener->onTakePhotoComplete(filePath);
    }else
    {
        ofLogWarning(__func__) << filePath << " WRITTEN BUT NO LISTENER SET";
    }
}

int ofxOMXPhotoGrabber::getWidth()
{
    return settings.width;
}

int ofxOMXPhotoGrabber::getHeight()
{
    return settings.height;
}


void ofxOMXPhotoGrabber::draw(ofRectangle& rectangle)
{
    displayController.draw(rectangle);
}

void ofxOMXPhotoGrabber::draw(int x, int y)
{
    displayController.draw(x, y);

}
void ofxOMXPhotoGrabber::draw(int x, int y, int width, int height)
{
    
    displayController.draw(x, y, width, height);
}


void ofxOMXPhotoGrabber::setDisplayAlpha(int alpha)
{
    displayController.setDisplayAlpha(alpha);
}

void ofxOMXPhotoGrabber::setDisplayLayer(int layer)
{
    displayController.setDisplayAlpha(layer);
}

void ofxOMXPhotoGrabber::setDisplayRotation(int rotationDegrees)
{
    displayController.setDisplayRotation(rotationDegrees);
}

void ofxOMXPhotoGrabber::setDisplayDrawRectangle(ofRectangle drawRectangle)
{
    displayController.setDisplayDrawRectangle(drawRectangle);
}

void ofxOMXPhotoGrabber::setDisplayCropRectangle(ofRectangle cropRectangle)
{
    displayController.setDisplayCropRectangle(cropRectangle);
}

void ofxOMXPhotoGrabber::setDisplayMirror(bool doMirror)
{
    displayController.setDisplayMirror(doMirror);
}


ofxOMXPhotoGrabber::~ofxOMXPhotoGrabber()
{
    listener = NULL;
}






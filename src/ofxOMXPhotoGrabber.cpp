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

void ofxOMXPhotoGrabber::setup(ofxOMXCameraSettings& settings_)
{
    settings = settings_;
    listener = settings.photoGrabberListener;
    
    
    if(settings.drawRectangle.isZero())
    {
        settings.drawRectangle.set(0, 0, settings.stillPreviewWidth, settings.stillPreviewHeight);
    }
    if(settings.cropRectangle.isZero())
    {
        settings.cropRectangle.set(0, 0, settings.width, settings.height);
    }
    ofLogNotice(__func__) << settings.toString();
   
    if(settings.enableTexture)
    {
        displayController.generateEGLImage(settings.stillPreviewWidth, settings.stillPreviewHeight);

    }
    engine.setup(&settings, this, displayController.eglImage);  


}


void ofxOMXPhotoGrabber::onPhotoEngineStart(OMX_HANDLETYPE camera_)
{
    camera = camera_;
    
    applyAllSettings();
    
    if(settings.enableTexture)
    {
        displayController.setup(&settings);
    }else
    {
        displayController.setup(&settings, engine.render);

    }
    engine.isOpen = true;
    
    
    
}


bool ofxOMXPhotoGrabber::isReady()
{
    return engine.isOpen;
}


int photoStart = 0;
void ofxOMXPhotoGrabber::takePhoto(int numShots)
{   
    shotsRequested+=numShots;
    if(camera)
    {
        photoStart = ofGetElapsedTimeMillis();
        engine.takePhoto();
    }else
    {
        ofLogError(__func__ ) << "NO CAMERA";
    }
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
    
    
    if(shotsTaken)
    {
        //setBurstMode(settings.burstModeEnabled);
        if(shotsTaken >= 1)
        {
            //arbitrary timing so exposure doesn't fade to black :/
            //ofSleepMillis(300);
        }
        if(shotsTaken < shotsRequested)
        {
            ofLogNotice(__func__) << "TAKING ANOTHER";
            takePhoto();
        }else
        {
            engine.stopCapture();
            //engine.close();
            //engine.setup(&settings, this, displayController.eglImage);
            ofLog() << shotsTaken << " TOOK totalTime: " << totalTime << " EACH " << totalTime/shotsTaken;
        }
    }
    
}

void ofxOMXPhotoGrabber::setJPEGCompression(int compression)
{
    if(compression <= 0)
    {
        compression = 1;
    }else
    {
        if(compression>100)
        {
            compression = 100;
        }
    }
    engine.setJPEGCompression(compression);
}

int ofxOMXPhotoGrabber::getWidth()
{
    return settings.stillPreviewWidth;
}

int ofxOMXPhotoGrabber::getHeight()
{
    return settings.stillPreviewHeight;
}


void ofxOMXPhotoGrabber::draw(ofRectangle& rectangle)
{
    if(engine.isCapturing) return;
    displayController.updateTexture();
    displayController.draw(rectangle.x, rectangle.y, rectangle.width, rectangle.height);
}

void ofxOMXPhotoGrabber::draw(int x, int y)
{
    if(engine.isCapturing) return;
    displayController.updateTexture();
    displayController.draw(x, y);  
}

void ofxOMXPhotoGrabber::draw(int x, int y, int width, int height)
{
    if(engine.isCapturing) return;
    displayController.updateTexture();
    displayController.draw(x, y, width, height);
}

void ofxOMXPhotoGrabber::draw()
{
    if(engine.isCapturing) return;
    displayController.updateTexture();
    displayController.draw();  
}




ofxOMXPhotoGrabber::~ofxOMXPhotoGrabber()
{
    close();
}

void ofxOMXPhotoGrabber::close()
{
    displayController.close();
    engine.close();
    camera = NULL;
    photosTaken.clear();
    listener = NULL;
    shotsRequested = 0;
    shotsTaken = 0;
}





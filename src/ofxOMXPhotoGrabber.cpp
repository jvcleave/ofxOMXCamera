#include "ofxOMXPhotoGrabber.h"

int shotsRequested;
int shotsTaken;

ofxOMXPhotoGrabber::ofxOMXPhotoGrabber()
{
    cameraOutputPort = CAMERA_STILL_OUTPUT_PORT;
    resetValues();
    camera = NULL;
    shotsRequested = 0;
    shotsTaken = 0;
    updateFrameCounter = 0;
    frameCounter = 0;
    hasNewFrame = false;
    
    ofAddListener(ofEvents().update, this, &ofxOMXPhotoGrabber::onUpdate);    

}

int totalTime = 0;

void ofxOMXPhotoGrabber::setup(ofxOMXCameraSettings& settings_)
{
    settings = settings_;
    settings.sensorWidth = VCOS_ALIGN_UP(settings.sensorWidth, 32);
    settings.sensorHeight = VCOS_ALIGN_UP(settings.sensorHeight, 16);
    settings.stillPreviewWidth = VCOS_ALIGN_UP(settings.stillPreviewWidth, 32);
    settings.stillPreviewHeight = VCOS_ALIGN_UP(settings.stillPreviewHeight, 16);
    listener = settings.photoGrabberListener;
    
    
    if(settings.drawRectangle.isZero())
    {
        settings.drawRectangle.set(0, 0, settings.stillPreviewWidth, settings.stillPreviewHeight);
    }
    if(settings.drawCropRectangle.isZero())
    {
        settings.drawCropRectangle.set(0, 0, settings.stillPreviewWidth, settings.stillPreviewHeight);
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
    imageFXController.setup(camera, OMX_ALL);
    if(settings.enableExtraVideoFilter)
    {
        extraImageFXController.setup(engine.imageFX, IMAGE_FX_OUTPUT_PORT);
    }
    applyAllSettings();
    
    if(settings.enableTexture)
    {
        displayController.setup(&settings);
    }else
    {
        displayController.setup(&settings, engine.render);

    }
    ofAddListener(ofEvents().update, this, &ofxOMXPhotoGrabber::onUpdate); 

    engine.isOpen = true;
    
    if(listener)
    {
        listener->onPhotoGrabberEngineStart();
    }
    
}


bool ofxOMXPhotoGrabber::isFrameNew()
{
    if(!settings.enableStillPreview) return false;
    
    if(!settings.enableTexture)
    {
        return  true;
    }
    return hasNewFrame;
}


#pragma mark UPDATE
void ofxOMXPhotoGrabber::onUpdate(ofEventArgs & args)
{
    
    if(!engine.isOpen)
    {
        //ofLogError() << "ENGINE CLOSED";
        return;
    }
    
    if(engine.isCapturing) return;

    if (settings.enableTexture && settings.enableStillPreview) 
    {
        frameCounter = engine.getFrameCounter();
        if (frameCounter > updateFrameCounter) 
        {
            updateFrameCounter = frameCounter;
            hasNewFrame = true;
            
        }else
        {
            hasNewFrame = false;
        }
        if (hasNewFrame) 
        {
            displayController.updateTexture(pixelsRequested);
        }
    }
    //ofLogVerbose() << "hasNewFrame: " << hasNewFrame;
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
#pragma mark PREVIEW RECORDING

void ofxOMXPhotoGrabber::setRecordingBitrate(float recordingBitrateMB)
{
    engine.videoRecorder.setRecordingBitrate(recordingBitrateMB);
}

bool ofxOMXPhotoGrabber::isRecording()
{
    bool result = false;
    if(settings.enableStillPreview)
    {
        result = engine.videoRecorder.isRecording;
    }
    return result;
}

void ofxOMXPhotoGrabber::startRecording()
{
    if(settings.enableStillPreview)
    {
        engine.videoRecorder.startRecording();
    }else
    {
        ofLogError(__func__) << "settings.enableStillPreview MUST BE ENABLED: " << settings.enableStillPreview;
    }
    
}

void ofxOMXPhotoGrabber::stopRecording()
{
    if(settings.enableStillPreview)
    {
        engine.videoRecorder.stopRecording();
    }else
    {
        ofLogError(__func__) << "settings.enableStillPreview MUST BE ENABLED: " << settings.enableStillPreview;
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
    displayController.draw(rectangle.x, rectangle.y, rectangle.width, rectangle.height);
}

void ofxOMXPhotoGrabber::draw(int x, int y)
{
    if(engine.isCapturing) return;
    displayController.draw(x, y);  
}

void ofxOMXPhotoGrabber::draw(int x, int y, int width, int height)
{
    if(engine.isCapturing) return;
    displayController.draw(x, y, width, height);
}

void ofxOMXPhotoGrabber::draw()
{
    if(engine.isCapturing) return;
    displayController.draw();  
}


ofxOMXPhotoGrabber::~ofxOMXPhotoGrabber()
{
    close();
}

void ofxOMXPhotoGrabber::close()
{
    
    ofRemoveListener(ofEvents().update, this, &ofxOMXPhotoGrabber::onUpdate);
    displayController.close();
    engine.close();
    camera = NULL;
    photosTaken.clear();
    listener = NULL;
    shotsRequested = 0;
    shotsTaken = 0;
}





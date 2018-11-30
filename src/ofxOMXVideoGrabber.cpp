#include "ofxOMXVideoGrabber.h"

#pragma mark SETUP
ofxOMXVideoGrabber::ofxOMXVideoGrabber()
{
    cameraOutputPort = CAMERA_OUTPUT_PORT;
    resetValues();
	updateFrameCounter = 0;
	frameCounter = 0;
	hasNewFrame = false;
    camera = NULL;
	ofAddListener(ofEvents().update, this, &ofxOMXVideoGrabber::onUpdate);    
}



void ofxOMXVideoGrabber::setup(ofxOMXCameraSettings& settings_)
{
    settings = settings_;
    //ofLogNotice(__func__) << settings.toJSON().dump();
    //ofLogNotice(__func__) << "settings: " << settings.toString();
    
    settings.sensorWidth = VCOS_ALIGN_UP(settings.sensorHeight, 32);
    settings.sensorHeight = VCOS_ALIGN_UP(settings.sensorHeight, 16);
    
    if(settings.drawRectangle.isZero())
    {
        settings.drawRectangle.set(0, 0, settings.sensorWidth, settings.sensorHeight);
    }
    if(settings.drawCropRectangle.isZero())
    {
        settings.drawCropRectangle.set(0, 0, settings.sensorWidth, settings.sensorHeight);
    }
    
    if(settings.enableTexture)
    {
        displayController.generateEGLImage(settings.sensorWidth, settings.sensorHeight);
        if(settings.enablePixels)
        {
            pixelsRequested = true;
        }
    }
    if(engine.isOpen)
    {
        engine.close();
        //ofSleepMillis(2000);
    }
    bool didOpen = engine.setup(&settings, this, displayController.eglImage);
    if(!didOpen)
    {
        ofLogError(__func__) << "COULD NOT OPEN ENGINE";
    }
    
}

void ofxOMXVideoGrabber::onVideoEngineStart()
{
    ofLogVerbose(__func__) << endl;

    camera = engine.camera;
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

    ofAddListener(ofEvents().update, this, &ofxOMXVideoGrabber::onUpdate); 
    engine.isOpen = true;
    
    
}

void ofxOMXVideoGrabber::onVideoEngineClose()
{
    ofLogVerbose(__func__) << endl;
}

void ofxOMXVideoGrabber::onRecordingComplete(string filePath)
{
    if(settings.videoGrabberListener)
    {
        settings.videoGrabberListener->onRecordingComplete(filePath);
    }else
    {
        ofLogWarning(__func__) << "RECEIVED " << filePath << " BUT NO LISTENER SET";
    }
}

bool ofxOMXVideoGrabber::isReady()
{
    return engine.isOpen;
}

#pragma mark UPDATE
void ofxOMXVideoGrabber::onUpdate(ofEventArgs & args)
{
    
    if(!engine.isOpen)
    {
        //ofLogError() << "ENGINE CLOSED";
        return;
    }
    if (settings.enableTexture) 
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



#pragma mark GETTERS

bool ofxOMXVideoGrabber::isFrameNew()
{
    if(!settings.enableTexture)
    {
        return  true;
    }
    return hasNewFrame;
}


int ofxOMXVideoGrabber::getWidth()
{
	return settings.sensorWidth;
}

int ofxOMXVideoGrabber::getHeight()
{
	return settings.sensorHeight;
}

int ofxOMXVideoGrabber::getFrameRate()
{
	return settings.framerate;
}




#pragma mark RECORDING

void ofxOMXVideoGrabber::setRecordingBitrate(float recordingBitrateMB)
{
    engine.videoRecorder.setRecordingBitrate(recordingBitrateMB);
}

bool ofxOMXVideoGrabber::isRecording()
{
    return engine.videoRecorder.isRecording;
}

void ofxOMXVideoGrabber::startRecording()
{
    engine.videoRecorder.startRecording();
}

void ofxOMXVideoGrabber::stopRecording()
{
	engine.videoRecorder.stopRecording();
}

#pragma mark DRAW
void ofxOMXVideoGrabber::draw(ofRectangle& rectangle)
{
    displayController.draw(rectangle.x, rectangle.y, rectangle.width, rectangle.height);
}

void ofxOMXVideoGrabber::draw(int x, int y)
{
    displayController.draw(x, y);  
}

void ofxOMXVideoGrabber::draw(int x, int y, int width, int height)
{
    displayController.draw(x, y, width, height);
}

void ofxOMXVideoGrabber::draw()
{
    displayController.draw();  
}


#pragma mark EXIT

void ofxOMXVideoGrabber::close()
{
    
    cout << "ofxOMXVideoGrabber::close" << endl;
    ofRemoveListener(ofEvents().update, this, &ofxOMXVideoGrabber::onUpdate);
    engine.close(); 
    cout << "~ofxOMXVideoGrabber::close END" << endl;
}

ofxOMXVideoGrabber::~ofxOMXVideoGrabber()
{
    cout << "~ofxOMXVideoGrabber" << endl;
    close();
}



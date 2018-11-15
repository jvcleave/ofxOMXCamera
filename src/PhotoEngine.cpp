#include "PhotoEngine.h"



#pragma mark TEXTURE RENDER CALLBACKS
OMX_ERRORTYPE PhotoEngine::textureRenderFillBufferDone(OMX_IN OMX_HANDLETYPE render, OMX_IN OMX_PTR photoEngine, OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{    
    return OMX_FillThisBuffer(render, pBuffer);
}



#pragma mark ENCODER CALLBACKS
OMX_ERRORTYPE 
PhotoEngine::encoderEventHandlerCallback(OMX_HANDLETYPE encoder, OMX_PTR photoEngine, OMX_EVENTTYPE event, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
     //ofLog() << "ENCODER: " << DebugEventHandlerString(encoder, event, nData1, nData2, pEventData);    
    if(event == OMX_EventBufferFlag)
    {
        PhotoEngine* engine = static_cast<PhotoEngine*>(photoEngine);
        engine->writeFile();
    }
    //PhotoEngine* engine = static_cast<PhotoEngine*>(photoEngine);
    
    //ofLogNotice(__func__) << GetEventString(event);
    return OMX_ErrorNone;
    
    
}


OMX_ERRORTYPE PhotoEngine::encoderFillBufferDone(OMX_HANDLETYPE encoder, OMX_PTR photoEngine, OMX_BUFFERHEADERTYPE* encoderOutputBuffer)
{    
    OMX_ERRORTYPE error = OMX_ErrorNone;

    if(encoderOutputBuffer->nFilledLen)
    {
        PhotoEngine* engine = static_cast<PhotoEngine*>(photoEngine);
        engine->recordingFileBuffer.append((const char*) encoderOutputBuffer->pBuffer + encoderOutputBuffer->nOffset, 
                                           encoderOutputBuffer->nFilledLen);
        //ofLogVerbose(__func__) << engine->recordingFileBuffer.size();
        error = OMX_FillThisBuffer(encoder, encoderOutputBuffer);
        OMX_TRACE(error); 
    }else
    {
        //ofLogError(__func__) << "encoderOutputBuffer->nFilledLen IS ZERO" << encoderOutputBuffer->nFilledLen;
    }
    return error;
}

PhotoEngine::PhotoEngine()
{
	isOpen = false;
    settings = NULL;
    render = NULL;
    camera = NULL;
    encoder = NULL;
    encoderOutputBuffer = NULL;
    listener = NULL;
    nullSink = NULL;
    eglImage = NULL;
    
    OMX_INIT_STRUCTURE(previewPortConfig);
    previewPortConfig.nPortIndex = CAMERA_PREVIEW_PORT;
    saveFolderAbsolutePath.clear();
    
    renderInputPort = VIDEO_RENDER_INPUT_PORT;
    isCapturing = false;
    
    OMX_INIT_STRUCTURE(cameraStillOutputPortConfig);
    cameraStillOutputPortConfig.nPortIndex = CAMERA_STILL_OUTPUT_PORT;
    
    OMX_INIT_STRUCTURE(encoderOutputPortDefinition);
    encoderOutputPortDefinition.nPortIndex = IMAGE_ENCODER_OUTPUT_PORT;
    
    encoderCallbacks.EventHandler       = &PhotoEngine::encoderEventHandlerCallback;
    encoderCallbacks.EmptyBufferDone    = &PhotoEngine::nullEmptyBufferDone;
    encoderCallbacks.FillBufferDone     = &PhotoEngine::encoderFillBufferDone;
    
    nullSinkCallbacks.EventHandler      = &PhotoEngine::nullEventHandlerCallback;
    nullSinkCallbacks.EmptyBufferDone   = &PhotoEngine::nullEmptyBufferDone;
    nullSinkCallbacks.FillBufferDone    = &PhotoEngine::nullFillBufferDone;
    
    renderCallbacks.EventHandler        = &PhotoEngine::nullEventHandlerCallback;
    renderCallbacks.EmptyBufferDone     = &PhotoEngine::nullEmptyBufferDone;
    renderCallbacks.FillBufferDone      = &PhotoEngine::nullFillBufferDone;
    
    cameraCallbacks.EventHandler        = &PhotoEngine::cameraEventHandlerCallback;
    cameraCallbacks.EmptyBufferDone     = &PhotoEngine::nullEmptyBufferDone;
    cameraCallbacks.FillBufferDone      = &PhotoEngine::nullFillBufferDone;
    
    OMX_INIT_STRUCTURE(cameraCallback);
    cameraCallback.nPortIndex           = OMX_ALL;
    cameraCallback.nIndex               = OMX_IndexParamCameraDeviceNumber;
    cameraCallback.bEnable              = OMX_TRUE;
    
    OMX_INIT_STRUCTURE(frameSizeConfig);
    frameSizeConfig.nPortIndex = OMX_ALL;
    
    OMX_INIT_STRUCTURE(sensorMode);
    sensorMode.nPortIndex = OMX_ALL;
    sensorMode.sFrameSize = frameSizeConfig;
    
    OMX_INIT_STRUCTURE(stillPortConfig);
    stillPortConfig.nPortIndex = CAMERA_STILL_OUTPUT_PORT;
    
    OMX_INIT_STRUCTURE(device);
    device.nPortIndex = OMX_ALL;
    
    OMX_INIT_STRUCTURE(compressionConfig);
    compressionConfig.nPortIndex = IMAGE_ENCODER_OUTPUT_PORT;
}   

void PhotoEngine::setup(ofxOMXCameraSettings* settings_, PhotoEngineListener* listener_, EGLImageKHR eglImage_)
{
    settings = settings_;
    if(!settings) return;
    
    listener = listener_;
    eglImage = eglImage_;
    
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    ofLogNotice(__func__) << settings->toString();

    error =OMX_GetHandle(&encoder, OMX_IMAGE_ENCODER, this , &encoderCallbacks);
    OMX_TRACE(error);
    
    error = DisableAllPortsForComponent(&encoder);
    OMX_TRACE(error);
    
    error =OMX_GetHandle(&nullSink, OMX_NULL_SINK, this , &nullSinkCallbacks);
    OMX_TRACE(error);
    
    error = DisableAllPortsForComponent(&nullSink);
    OMX_TRACE(error);
    
    
    OMX_STRING renderType = OMX_VIDEO_RENDER;
    if(settings->enableTexture)
    {
        renderType = OMX_EGL_RENDER; 
        renderInputPort = EGL_RENDER_INPUT_PORT;
    }else
    {
         
        renderInputPort = VIDEO_RENDER_INPUT_PORT;
    }
    if(settings->enableStillPreview) 
    {
        if(settings->enableTexture)
        {
            renderCallbacks.FillBufferDone    = &PhotoEngine::textureRenderFillBufferDone;
        }else
        {
            renderCallbacks.FillBufferDone    = &PhotoEngine::nullFillBufferDone;
        }
        error = OMX_GetHandle(&render, renderType, this , &renderCallbacks);
        OMX_TRACE(error);
        
        error = DisableAllPortsForComponent(&render);
        OMX_TRACE(error);
        
    }
    
    //create Camera
    error = OMX_GetHandle(&camera, OMX_CAMERA, this , &cameraCallbacks);
    OMX_TRACE(error);
    
    error = DisableAllPortsForComponent(&camera);
    OMX_TRACE(error);
    
    error = OMX_SetConfig(camera, OMX_IndexConfigRequestCallback, &cameraCallback);
    OMX_TRACE(error);
    
    //Set the camera (usually 0 unless compute module/multiple cameras)
    device.nU32         = settings->cameraDeviceID;
    
    error = OMX_SetParameter(camera, OMX_IndexParamCameraDeviceNumber, &device);
    OMX_TRACE(error);
    

    if(settings->enableStillPreview) 
    { 
        error =  OMX_GetParameter(camera, OMX_IndexParamPortDefinition, &previewPortConfig);
        OMX_TRACE(error);
        
        previewPortConfig.format.video.nFrameWidth  = settings->stillPreviewWidth;
        previewPortConfig.format.video.nFrameHeight = settings->stillPreviewHeight;
        previewPortConfig.format.video.nStride      = settings->stillPreviewWidth;
        //not setting also works
        previewPortConfig.format.video.nSliceHeight    = settings->stillPreviewHeight;
        
        error =  OMX_SetParameter(camera, OMX_IndexParamPortDefinition, &previewPortConfig);
        OMX_TRACE(error);
    }
    
    //Enable RAW info embedded in JPEG
    //may need https://github.com/6by9/dcraw to make useful
    if(settings->enableRaw)
    {
        char dummy[] = "dummy";
        struct {
            //These two fields need to be together
            OMX_PARAM_CONTENTURITYPE rawConfig;
            char padding[5];
        } raw;
        OMX_INIT_STRUCTURE(raw.rawConfig);
        memcpy (raw.rawConfig.contentURI, dummy, 5);
        raw.rawConfig.nSize = sizeof (raw);
        error =  OMX_SetParameter(camera, OMX_IndexConfigCaptureRawImageURI, &raw);
        OMX_TRACE(error);
    }
    

    error =OMX_GetParameter(camera, OMX_IndexParamCommonSensorMode, &sensorMode);
    OMX_TRACE(error);
    
    sensorMode.bOneShot = OMX_TRUE; //seems to determine whether OMX_BUFFERFLAG_EOS is passed    
    error =OMX_SetParameter(camera, OMX_IndexParamCommonSensorMode, &sensorMode);
    OMX_TRACE(error);
    
    //Set the resolution    
    error =  OMX_GetParameter(camera, OMX_IndexParamPortDefinition, &stillPortConfig);
    OMX_TRACE(error);
    
    stillPortConfig.format.image.nFrameWidth        = settings->width;
    stillPortConfig.format.image.nFrameHeight       = settings->height;
    stillPortConfig.format.image.nStride            = settings->width;
    stillPortConfig.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
    stillPortConfig.format.image.eColorFormat       = OMX_COLOR_FormatUnused;
    stillPortConfig.format.image.nStride            = settings->width;
    
    //ofLog() << "CAMERA_STILL_OUTPUT_PORT DEFAULT: " << GetColorFormatString(stillPortConfig.format.image.eColorFormat);
    //DEFAULT: YUV420PackedPlanar
    //YUV420PackedSemiPlanar
    //Unused
    //YUV420PackedPlanar
    //YUV420PackedSemiPlanar
    //YUV422PackedPlanar
    //YVU420PackedPlanar
    //YVU420PackedSemiPlanar
    error =  OMX_SetParameter(camera, OMX_IndexParamPortDefinition, &stillPortConfig);
    OMX_TRACE(error);
}

#pragma mark CAMERA CALLBACKS
OMX_ERRORTYPE 
PhotoEngine::cameraEventHandlerCallback(OMX_HANDLETYPE hComponent, OMX_PTR photoEngine, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
    
    PhotoEngine* engine = static_cast<PhotoEngine*>(photoEngine);
    
    //ofLog() << "CAMERA: " << DebugEventHandlerString(hComponent, eEvent, nData1, nData2, pEventData);
    
    if(eEvent == OMX_EventParamOrConfigChanged)
    {
        return engine->onCameraEventParamOrConfigChanged();
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PhotoEngine::onCameraEventParamOrConfigChanged()
{
    
    OMX_ERRORTYPE error = SetComponentState(camera, OMX_StateIdle);
    OMX_TRACE(error);
    
    //PrintSensorModes(camera);
    ofLogNotice(__func__) << settings->toString();

    if(settings->enableStillPreview) 
    { 
        
        //Set renderer to Idle
        error = SetComponentState(render, OMX_StateIdle);
        OMX_TRACE(error);
        
        if(settings->enableTexture)
        {
            //Enable render output port
            error = EnableComponentPort(render, EGL_RENDER_OUTPUT_PORT);
            OMX_TRACE(error);
        }
        
        if(settings->enableTexture)
        {
            //Set renderer to use texture
            error = OMX_UseEGLImage(render, &eglBuffer, EGL_RENDER_OUTPUT_PORT, this, eglImage);
            OMX_TRACE(error);
        }
        
        error = OMX_SetupTunnel(camera, CAMERA_PREVIEW_PORT, render, renderInputPort);
        OMX_TRACE(error);
        
        if(renderInputPort == EGL_RENDER_INPUT_PORT)
        {
            ofLogNotice(__func__) << "USING EGL_RENDER_INPUT_PORT";
        }
        if(renderInputPort == VIDEO_RENDER_INPUT_PORT)
        {
            ofLogNotice(__func__) << "USING VIDEO_RENDER_INPUT_PORT";
        }
        //Enable camera preview port
        error = WaitForPortEnable(camera, CAMERA_PREVIEW_PORT);
        OMX_TRACE(error);
        
        
        //Enable render input port
        error = WaitForPortEnable(render, renderInputPort);
        OMX_TRACE(error);
        
        
    }else
    {
        
        /*
         From raspistill:
         If preview is disabled, the null_sink component is used to 'absorb' the preview frames.
         It is necessary for the camera to produce preview frames even if not required for display,
         as they are used for calculating exposure and white balance settings->
         */
        error = OMX_SetupTunnel(camera, CAMERA_PREVIEW_PORT, nullSink, NULL_SINK_INPUT_PORT);
        OMX_TRACE(error);
        
        //Enable camera preview port
        error = WaitForPortEnable(camera, CAMERA_PREVIEW_PORT);
        OMX_TRACE(error);
        
        //Enable nullSink input port
        error = WaitForPortEnable(nullSink, NULL_SINK_INPUT_PORT);
        OMX_TRACE(error);
        
        //Start renderer
        error = SetComponentState(nullSink, OMX_StateExecuting);
        OMX_TRACE(error);
    }
    
    
    error = SetComponentState(encoder, OMX_StateIdle);
    OMX_TRACE(error);
    
    //config encoder

    error =OMX_GetParameter(encoder, OMX_IndexParamPortDefinition, &encoderOutputPortDefinition);
    OMX_TRACE(error);
    
    encoderOutputPortDefinition.format.image.nFrameWidth        = settings->width;
    encoderOutputPortDefinition.format.image.nFrameHeight       = settings->height;
    encoderOutputPortDefinition.format.image.nSliceHeight       = settings->height;
    encoderOutputPortDefinition.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
    encoderOutputPortDefinition.format.image.eColorFormat       = OMX_COLOR_FormatUnused;
    
    error =OMX_SetParameter(encoder, OMX_IndexParamPortDefinition, &encoderOutputPortDefinition);
    OMX_TRACE(error);
    
    encoderOutputBufferSize = encoderOutputPortDefinition.nBufferSize;
    

    setJPEGCompression(settings->stillQuality);
    
  

    
    //Create camera->encoder Tunnel
    error = OMX_SetupTunnel(camera, CAMERA_STILL_OUTPUT_PORT, encoder, IMAGE_ENCODER_INPUT_PORT);
    OMX_TRACE(error);
    
    //Enable camera output port
    error = WaitForPortEnable(camera, CAMERA_STILL_OUTPUT_PORT);
    OMX_TRACE(error);
    
    //Enable encoder input port
    error = WaitForPortEnable(encoder, IMAGE_ENCODER_INPUT_PORT);    
    OMX_TRACE(error); 
    
    //Enable encoder output port
    error = WaitForPortEnable(encoder, IMAGE_ENCODER_OUTPUT_PORT);
    OMX_TRACE(error); 
    
    //Enable encoderOutputBuffer
    error =  OMX_AllocateBuffer(encoder, &encoderOutputBuffer, IMAGE_ENCODER_OUTPUT_PORT, NULL, encoderOutputBufferSize);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone)
    {
        ofLogError(__func__) << "FAILED ALLOCATED AT STATE: " << PrintOMXState(encoder);
    }
    
    
    //Start camera
    error = SetComponentState(camera, OMX_StateExecuting);
    OMX_TRACE(error);  
    
    if(settings->enableStillPreview) 
    { 
        //Start renderer
        error = SetComponentState(render, OMX_StateExecuting);
        OMX_TRACE(error);
 
        if(settings->enableTexture)
        {
            //start the buffer filling loop
            //once completed the callback will trigger and refill
            error = OMX_FillThisBuffer(render, eglBuffer);
            OMX_TRACE(error);
            if(error == OMX_ErrorNone)
            {
                ofLogNotice(__func__) << "TRIED OMX_FillThisBuffer";
 
            }
            
        }
    }
    

    //Start encoder
    error = WaitForState(encoder, OMX_StateExecuting);
    OMX_TRACE(error);
    
    listener->onPhotoEngineStart(camera);
    
    return error;
}
void PhotoEngine::setJPEGCompression(int quality)
{
    if(!settings) return;
    
    OMX_ERRORTYPE error =OMX_GetParameter(encoder, OMX_IndexParamQFactor, &compressionConfig);
    OMX_TRACE(error);
    
    compressionConfig.nQFactor = quality;
    
    error = OMX_SetParameter(encoder, OMX_IndexParamQFactor, &compressionConfig);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone)
    {
        settings->stillQuality = quality;
    }
}

void PhotoEngine::stopCapture()
{
    OMX_ERRORTYPE error;

    cameraStillOutputPortConfig.bEnabled = OMX_FALSE;
    
    error =OMX_SetParameter(camera, OMX_IndexConfigPortCapturing, &cameraStillOutputPortConfig);    
    OMX_TRACE(error);
    isCapturing = false;
}

void PhotoEngine::takePhoto()
{
    ofLogVerbose(__func__);
    OMX_ERRORTYPE error;
    
    if(!isCapturing)
    {
        isCapturing = true;
        //Set camera to Idle

    }

    cameraStillOutputPortConfig.bEnabled = OMX_TRUE;
    error =OMX_SetParameter(camera, OMX_IndexConfigPortCapturing, &cameraStillOutputPortConfig);    
    OMX_TRACE(error);
    
    //Start capturing
    error = OMX_FillThisBuffer(encoder, encoderOutputBuffer);
    OMX_TRACE(error);
    
    
    if (error != OMX_ErrorNone) 
    {
        ofLogError() << "TAKE PHOTO FAILED";
    }
}

void PhotoEngine::writeFile()
{
        
    //OMX_ERRORTYPE error;
    
    bool result = false;
    
    if(saveFolderAbsolutePath.empty())
    {
        ofDirectory saveFolder;
        
        
        if(!settings->savedPhotosFolderName.empty())
        {
            saveFolder = ofDirectory(ofToDataPath(settings->savedPhotosFolderName, true));
            
        }else
        {
            saveFolder = ofDirectory(ofToDataPath("", true)); 
            
        }
        if(!saveFolder.exists())
        {
            saveFolder.create();
        }
        saveFolderAbsolutePath = saveFolder.getAbsolutePath();
    }
    
    string filePath = saveFolderAbsolutePath + "/" + ofGetTimestampString()+"_Q" + ofToString(settings->stillQuality)+".jpg";
    
    if(recordingFileBuffer.size()>0)
    {
        LINE_TIME_START
        result = ofBufferToFile(filePath, recordingFileBuffer);
        LINE_TIME_END("ofBufferToFile")
    }
    recordingFileBuffer.clear();
    
    
    if(result)
    {
        if(listener)
        {
            listener->onTakePhotoComplete(filePath);
        }else
        {
            ofLogWarning(__func__) << filePath << " WRITTEN BUT NO LISTENER SET";
        }
    }

}

PhotoEngine::~PhotoEngine()
{
    if(isOpen)
    {
        close();
    }
    settings = NULL;
}


void PhotoEngine::close()
{
    OMX_ERRORTYPE error;
    
    if(camera)
    {
        error = DisableAllPortsForComponent(&camera);
    }
    
    if(encoder)
    {
        error = DisableAllPortsForComponent(&encoder);
    }

    if(render)
    {
        error = DisableAllPortsForComponent(&render);
        //error = FlushOMXComponent(render, renderInputPort);
        OMX_TRACE(error);
        
        if(settings && settings->enableTexture)
        {
            error = FlushOMXComponent(render, EGL_RENDER_OUTPUT_PORT);
            OMX_TRACE(error);
            
        }
    }
    
    if(encoder)
    {
        if(encoderOutputBuffer)
        {
            error = OMX_FreeBuffer(encoder, IMAGE_ENCODER_OUTPUT_PORT, encoderOutputBuffer);
            OMX_TRACE(error);
            encoderOutputBuffer = NULL;
        }
    }
    
    if(camera)
    {
        error = OMX_SetupTunnel(camera, CAMERA_STILL_OUTPUT_PORT, NULL, 0);
        OMX_TRACE(error);
    }
    
    if(encoder)
    {
        error = OMX_SetupTunnel(encoder, IMAGE_ENCODER_INPUT_PORT, NULL, 0);
        OMX_TRACE(error);
    }
    
    if(settings && settings->enableStillPreview) 
    {    
        if(camera)
        {
            error = OMX_SetupTunnel(camera, CAMERA_PREVIEW_PORT, NULL, 0);
            OMX_TRACE(error);
        }
        
        if(render)
        {
            error = OMX_SetupTunnel(render, renderInputPort, NULL, 0);
            OMX_TRACE(error);
            
            error =  OMX_FreeHandle(render);
            OMX_TRACE(error);
            render = NULL;
        }
    }
    
    if(camera)
    {
        error = OMX_FreeHandle(camera);
        OMX_TRACE(error);
        camera = NULL;
    }
    
    if(encoder)
    {
        error = OMX_FreeHandle(encoder);
        OMX_TRACE(error); 
        encoder = NULL;
    }
    
    if(nullSink)
    {
        error = OMX_FreeHandle(nullSink);
        OMX_TRACE(error); 
        nullSink = NULL;
    }

    isOpen = false;
}



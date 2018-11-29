#include "VideoEngine.h"



VideoEngine::VideoEngine()
{
    isOpen        = false;
    stopRequested = false;     
    isStopping = false;
    isRecording = false;
    didWriteFile = false;
    recordedFrameCounter = 0;
    listener = NULL;
    frameCounter = 0;
    settings = NULL;
    eglBuffer = NULL;
    renderType = OMX_VIDEO_RENDER; 
    renderInputPort = VIDEO_RENDER_INPUT_PORT;
    

}

int VideoEngine::getFrameCounter()
{
    return frameCounter;
}

OMX_ERRORTYPE VideoEngine::textureRenderFillBufferDone(OMX_HANDLETYPE render, OMX_PTR videoEngine, OMX_BUFFERHEADERTYPE* pBuffer)
{    
    OMX_ERRORTYPE error = OMX_ErrorNone;

    VideoEngine* engine = static_cast<VideoEngine*>(videoEngine);
    if(engine->isOpen)
    {
        engine->frameCounter++;
        error = OMX_FillThisBuffer(render, pBuffer);
        OMX_TRACE(error);
    }
    return error;
}

OMX_ERRORTYPE VideoEngine::cameraEventHandlerCallback(OMX_HANDLETYPE camera, OMX_PTR videoEngine,
                                                      OMX_EVENTTYPE eEvent, OMX_U32 nData1,
                                                      OMX_U32 nData2, OMX_PTR pEventData)
{
    if(eEvent == OMX_EventParamOrConfigChanged)
    {
        VideoEngine* engine = static_cast<VideoEngine*>(videoEngine);
        return engine->onCameraEventParamOrConfigChanged();
    }
    return OMX_ErrorNone;
}






OMX_ERRORTYPE VideoEngine::setRecordingBitrate(float recordingBitrateMB_)
{
    OMX_VIDEO_CONFIG_BITRATETYPE bitrateConfig;
    OMX_INIT_STRUCTURE(bitrateConfig);
    bitrateConfig.nPortIndex = VIDEO_ENCODE_OUTPUT_PORT;
    bitrateConfig.nEncodeBitrate = MEGABYTE_IN_BITS * recordingBitrateMB_;
    
    OMX_ERRORTYPE error = OMX_SetConfig(encoder, OMX_IndexConfigVideoBitrate, &bitrateConfig);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone)
    {
        settings->recordingBitrateMB = recordingBitrateMB_;
        ofLogNotice(__func__) << "settings->recordingBitrateMB: " << settings->recordingBitrateMB;
    }
    return error;
}

bool VideoEngine::setup(ofxOMXCameraSettings* settings_, VideoEngineListener* listener_, EGLImageKHR eglImage_)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    bool success = false;
    settings = settings_;
    listener = listener_;
    eglImage = eglImage_;
    
    ofLogVerbose(__func__) << "settings: " << settings->toString();

    if(settings->enableExtraVideoFilter)
    {
        OMX_CALLBACKTYPE imageFXCallbacks;
        imageFXCallbacks.EventHandler       = &VideoEngine::nullEventHandler;
        imageFXCallbacks.EmptyBufferDone    = &VideoEngine::nullEmptyBufferDone;
        imageFXCallbacks.FillBufferDone     = &VideoEngine::nullFillBufferDone;
        
        error =OMX_GetHandle(&imageFX, OMX_IMAGE_FX, this , &imageFXCallbacks);
        OMX_TRACE(error);
        
        
        error = DisableAllPortsForComponent(&imageFX);
        OMX_TRACE(error);
    }



    
#pragma mark RENDER SETUP  

    if(settings->enableTexture)
    {
        renderType      = OMX_EGL_RENDER; 
        renderInputPort = EGL_RENDER_INPUT_PORT;
    }else
    {
        renderType      = OMX_VIDEO_RENDER; 
        renderInputPort = VIDEO_RENDER_INPUT_PORT;
    }
    
    ofLogVerbose(__func__) << "renderType: " << renderType << " : " << renderInputPort;

    OMX_CALLBACKTYPE renderCallbacks;
    renderCallbacks.EventHandler    = &VideoEngine::nullEventHandler;
    
    
    if(settings->enableTexture)
    {
        //Implementation specific
        renderCallbacks.FillBufferDone  = &VideoEngine::textureRenderFillBufferDone;
        renderCallbacks.EmptyBufferDone = &VideoEngine::nullEmptyBufferDone;

    }else
    {
        renderCallbacks.FillBufferDone  = &VideoEngine::nullFillBufferDone;
        renderCallbacks.EmptyBufferDone = &VideoEngine::nullEmptyBufferDone;

    }

    error = OMX_GetHandle(&render, renderType, this , &renderCallbacks);
    OMX_TRACE(error);
    
    error = DisableAllPortsForComponent(&render);
    OMX_TRACE(error);
    
    
#pragma mark SPLITTER SETUP  

    //Set up video splitter
    OMX_CALLBACKTYPE splitterCallbacks;
    splitterCallbacks.EventHandler    = &VideoEngine::nullEventHandler;
    splitterCallbacks.EmptyBufferDone = &VideoEngine::nullEmptyBufferDone;
    splitterCallbacks.FillBufferDone  = &VideoEngine::nullFillBufferDone;
    
    error = OMX_GetHandle(&splitter, OMX_VIDEO_SPLITTER, this , &splitterCallbacks);
    OMX_TRACE(error);
    error =DisableAllPortsForComponent(&splitter);
    OMX_TRACE(error);
    
    
#pragma mark CAMERA SETUP  

    OMX_CALLBACKTYPE cameraCallbacks;
    cameraCallbacks.EventHandler    = &VideoEngine::cameraEventHandlerCallback;
    cameraCallbacks.EmptyBufferDone = &VideoEngine::nullEmptyBufferDone;
    cameraCallbacks.FillBufferDone  = &VideoEngine::nullFillBufferDone;

    error = OMX_GetHandle(&camera, OMX_CAMERA, this , &cameraCallbacks);
    OMX_TRACE(error);

    error = DisableAllPortsForComponent(&camera);
    OMX_TRACE(error);
    
    OMX_CONFIG_REQUESTCALLBACKTYPE cameraCallback;
    OMX_INIT_STRUCTURE(cameraCallback);
    cameraCallback.nPortIndex    =    OMX_ALL;
    cameraCallback.nIndex        =    OMX_IndexParamCameraDeviceNumber;
    cameraCallback.bEnable        =    OMX_TRUE;
    
    error = OMX_SetConfig(camera, OMX_IndexConfigRequestCallback, &cameraCallback);
    OMX_TRACE(error);
    
    OMX_PARAM_U32TYPE device;
    OMX_INIT_STRUCTURE(device);
    device.nPortIndex   = OMX_ALL;
    device.nU32         = settings->cameraDeviceID;
    
    error = OMX_SetParameter(camera, OMX_IndexParamCameraDeviceNumber, &device);
    OMX_TRACE(error);
    
    
    //Set the resolution
    OMX_PARAM_PORTDEFINITIONTYPE cameraOutputPortDefinition;
    OMX_INIT_STRUCTURE(cameraOutputPortDefinition);
    cameraOutputPortDefinition.nPortIndex = CAMERA_OUTPUT_PORT;
    
    error =  OMX_GetParameter(camera, OMX_IndexParamPortDefinition, &cameraOutputPortDefinition);
    OMX_TRACE(error);
    
    
    cameraOutputPortDefinition.format.video.nFrameWidth     = settings->sensorWidth;
    cameraOutputPortDefinition.format.video.nFrameHeight    = settings->sensorHeight;
    cameraOutputPortDefinition.format.video.xFramerate      = settings->framerate << 16;
    cameraOutputPortDefinition.format.video.nStride         = settings->sensorWidth;
    //cameraOutputPortDefinition.format.video.eColorFormat    = OMX_COLOR_FormatYUV420PackedPlanar;
    cameraOutputPortDefinition.format.video.nSliceHeight    = settings->sensorHeight;
    
    error =  OMX_SetParameter(camera, OMX_IndexParamPortDefinition, &cameraOutputPortDefinition);
    OMX_TRACE(error);
    if(error == OMX_ErrorBadParameter)
    {
        ofLogError(__func__) << "USING FALLBACK CONFIG";
        settings->sensorWidth = 1280;
        settings->sensorHeight = 720;
        settings->framerate = 30;
        
        cameraOutputPortDefinition.format.video.nFrameWidth     = settings->sensorWidth;
        cameraOutputPortDefinition.format.video.nFrameHeight    = settings->sensorHeight;
        cameraOutputPortDefinition.format.video.xFramerate      = settings->framerate << 16;
        cameraOutputPortDefinition.format.video.nStride         = settings->sensorWidth;
        cameraOutputPortDefinition.format.video.nSliceHeight    = settings->sensorHeight;
        
        error =  OMX_SetParameter(camera, OMX_IndexParamPortDefinition, &cameraOutputPortDefinition);
        OMX_TRACE(error);
        if(error == OMX_ErrorBadParameter)
        {
            //return false; 
        }
    }
    
    
#pragma mark imageFX SETUP  
    
    if(settings->enableExtraVideoFilter)
    {

        
        OMX_PARAM_PORTDEFINITIONTYPE imageFXPortDefinition;
        OMX_INIT_STRUCTURE(imageFXPortDefinition);
        imageFXPortDefinition.nPortIndex = IMAGE_FX_INPUT_PORT;
        
        error =  OMX_GetParameter(imageFX, OMX_IndexParamPortDefinition, &imageFXPortDefinition);
        OMX_TRACE(error);
        imageFXPortDefinition.eDomain = OMX_PortDomainVideo;
        imageFXPortDefinition.format.video = cameraOutputPortDefinition.format.video;
        imageFXPortDefinition.format.video.nSliceHeight = settings->sensorHeight;
        
        error =  OMX_SetParameter(imageFX, OMX_IndexParamPortDefinition, &imageFXPortDefinition);
        OMX_TRACE(error);
        if(error == OMX_ErrorNone)
        {
            ofLog() << "IMAGE_FX_INPUT_PORT PASSED";
        }
        
        imageFXPortDefinition.nPortIndex = IMAGE_FX_OUTPUT_PORT;
        error =  OMX_SetParameter(imageFX, OMX_IndexParamPortDefinition, &imageFXPortDefinition);
        OMX_TRACE(error);
        if(error == OMX_ErrorNone)
        {
            ofLog() << "IMAGE_FX_OUTPUT_PORT PASSED";
        }
        
        /*
        OMX_PARAM_U32TYPE extra_buffers;
        OMX_INIT_STRUCTURE(extra_buffers);
        extra_buffers.nU32 = 10;
        
        error = OMX_SetParameter(imageFX, OMX_IndexParamBrcmExtraBuffers, &extra_buffers);
        OMX_TRACE(error);*/
    }
    
    
  
    
    //Enable Camera Output Port
    OMX_CONFIG_PORTBOOLEANTYPE cameraport;
    OMX_INIT_STRUCTURE(cameraport);
    cameraport.nPortIndex = CAMERA_OUTPUT_PORT;
    cameraport.bEnabled = OMX_TRUE;
    
    error =OMX_SetParameter(camera, OMX_IndexConfigPortCapturing, &cameraport);    
    OMX_TRACE(error);
    
    
    success = true;
    return success;
    //camera color spaces
    /*
     OMX_COLOR_Format24bitRGB888
     OMX_COLOR_FormatYUV420PackedPlanar
     OMX_COLOR_FormatYUV422PackedPlanar
     OMX_COLOR_FormatYCbYCr
     OMX_COLOR_FormatYCrYCb
     OMX_COLOR_FormatCbYCrY
     OMX_COLOR_FormatCrYCbY
     OMX_COLOR_FormatYUV420PackedSemiPlanar
     */
    
    //egl_render color spaces
    /*
     OMX_COLOR_Format18bitRGB666
     OMX_COLOR_FormatYUV420PackedPlanar
     OMX_COLOR_FormatYUV422PackedPlanar
     OMX_COLOR_Format32bitABGR8888
     */
    
}



OMX_ERRORTYPE VideoEngine::onCameraEventParamOrConfigChanged()
{
    OMX_ERRORTYPE error;
    error = SetComponentState(camera, OMX_StateIdle);
    OMX_TRACE(error);
    
    //Set splitter to Idle
    error = SetComponentState(splitter, OMX_StateIdle);
    OMX_TRACE(error);
    
    //Set renderer to Idle
    error = SetComponentState(render, OMX_StateIdle);
    OMX_TRACE(error);
    

    
    if(settings->enableExtraVideoFilter)
    {
        //Set imageFX to Idle
        error = WaitForState(imageFX, OMX_StateIdle);
        OMX_TRACE(error);
    }
    


#pragma mark TUNNELS SETUP  

    
    
    if(settings->enableExtraVideoFilter)
    {
        //Create camera->imageFX Tunnel
        error = OMX_SetupTunnel(camera, CAMERA_OUTPUT_PORT,
                                imageFX, IMAGE_FX_INPUT_PORT);
        OMX_TRACE(error);
        
        
        //Create imageFX->splitter Tunnel
        error = OMX_SetupTunnel(imageFX, IMAGE_FX_OUTPUT_PORT,
                                splitter, VIDEO_SPLITTER_INPUT_PORT);
        OMX_TRACE(error);

    }else
    {
        //Create camera->splitter Tunnel
        error = OMX_SetupTunnel(camera, CAMERA_OUTPUT_PORT,
                                splitter, VIDEO_SPLITTER_INPUT_PORT);
        OMX_TRACE(error);
    }
    

    //Create splitter->render Tunnel
    error = OMX_SetupTunnel(splitter, VIDEO_SPLITTER_OUTPUT_PORT1,
                            render, renderInputPort);
    OMX_TRACE(error);
    
    

   
    
    //Enable camera output port
    error = EnableComponentPort(camera, CAMERA_OUTPUT_PORT);
    OMX_TRACE(error);
    
    //Enable splitter input port
    error = EnableComponentPort(splitter, VIDEO_SPLITTER_INPUT_PORT);
    OMX_TRACE(error);
    
    //Enable splitter output port
    error = EnableComponentPort(splitter, VIDEO_SPLITTER_OUTPUT_PORT1);
    OMX_TRACE(error);
    

    
    
    
    if(settings->enableExtraVideoFilter)
    {
        //Enable imageFX
        error = EnableComponentPort(imageFX, IMAGE_FX_INPUT_PORT);
        OMX_TRACE(error);
        
        error = EnableComponentPort(imageFX, IMAGE_FX_OUTPUT_PORT);
        OMX_TRACE(error);
    }
    
    
    if(settings->enableTexture)
    {
        //Enable render output port
        error = EnableComponentPort(render, EGL_RENDER_OUTPUT_PORT);
        OMX_TRACE(error);
    }
    
    //Enable render input port
    error = EnableComponentPort(render, renderInputPort);
    OMX_TRACE(error);
    
    if(settings->enableTexture)
    {
        if(!eglImage)
        {
            ofLogError(__func__) << "displayController->eglImage IS NULL";
            
        }
        //Set renderer to use texture
        error = OMX_UseEGLImage(render, &eglBuffer, EGL_RENDER_OUTPUT_PORT, this, eglImage);
        OMX_TRACE(error);
    }
    
    
    //Start camera
    error = WaitForState(camera, OMX_StateExecuting);
    OMX_TRACE(error);
    
    //Start splitter
    error = WaitForState(splitter, OMX_StateExecuting);
    OMX_TRACE(error);
    
    if(settings->enableExtraVideoFilter)
    {
        //Start imageFX
        error = WaitForState(imageFX, OMX_StateExecuting);
        OMX_TRACE(error);
    }
    
    
    //Start renderer
    error = WaitForState(render, OMX_StateExecuting);
    OMX_TRACE(error);
    
    if(settings->enableTexture)
    {
        //start the buffer filling loop
        //once completed the callback will trigger and refill
        error = OMX_FillThisBuffer(render, eglBuffer);
        OMX_TRACE(error);
        if(error == OMX_ErrorIncorrectStateOperation)
        {
            
            OMX_STATETYPE currentState;
            error = OMX_GetState(render, &currentState);
            OMX_TRACE(error);  
            
            ofLogError() << "render currentState: " << GetOMXStateString(currentState);
        }
    }
    
#pragma mark ENGINE START
#if 0
    ofLogNotice(__func__) << "camera CAMERA_OUTPUT_PORT: " << PrintPortDefinition(camera, CAMERA_OUTPUT_PORT);
    if(settings->enableExtraVideoFilter)
    {
        ofLogNotice(__func__) << "imageFX IMAGE_FX_INPUT_PORT: " << PrintPortDefinition(imageFX, IMAGE_FX_INPUT_PORT);
        ofLogNotice(__func__) << "imageFX IMAGE_FX_OUTPUT_PORT: " << PrintPortDefinition(imageFX, IMAGE_FX_OUTPUT_PORT); 
    }
    ofLogNotice(__func__) << "splitter VIDEO_SPLITTER_INPUT_PORT: " << PrintPortDefinition(splitter, VIDEO_SPLITTER_INPUT_PORT);
    ofLogNotice(__func__) << "splitter VIDEO_SPLITTER_OUTPUT_PORT1: " << PrintPortDefinition(splitter, VIDEO_SPLITTER_OUTPUT_PORT1);
    ofLogNotice(__func__) << "splitter VIDEO_SPLITTER_OUTPUT_PORT2: " << PrintPortDefinition(splitter, VIDEO_SPLITTER_OUTPUT_PORT2);
    ofLogNotice(__func__) << "render renderInputPort: " << PrintPortDefinition(render, renderInputPort);
    if(settings->enableTexture)
    {
        ofLogNotice(__func__) << "render EGL_RENDER_OUTPUT_PORT: " << PrintPortDefinition(render, EGL_RENDER_OUTPUT_PORT);

    }
#endif
    
   
    
    listener->onVideoEngineStart();

    return error;
}

OMX_ERRORTYPE VideoEngine::encoderEventHandlerCallback(OMX_HANDLETYPE encoder, OMX_PTR videoEngine,
                                                       OMX_EVENTTYPE event, OMX_U32 nData1,
                                                       OMX_U32 nData2, OMX_PTR pEventData)
{
    
    ofLog() << "ENCODER: " << DebugEventHandlerString(encoder, event, nData1, nData2, pEventData); 
    if(event == OMX_EventBufferFlag)
    {
        VideoEngine* engine = static_cast<VideoEngine*>(videoEngine);
        //engine->writeFile();
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoEngine::encoderFillBufferDone(OMX_HANDLETYPE encoder, OMX_PTR videoEngine, OMX_BUFFERHEADERTYPE* encoderOutputBuffer)
{    
    VideoEngine* engine = static_cast<VideoEngine*>(videoEngine);
    
    bool isKeyframeValid = false;
    engine->recordedFrameCounter++;
    // The user wants to quit, but don't exit
    // the loop until we are certain that we have processed
    // a full frame till end of the frame, i.e. we're at the end
    // of the current key frame if processing one or until
    // the next key frame is detected. This way we should always
    // avoid corruption of the last encoded at the expense of
    // small delay in exiting.
    if(engine->stopRequested && !engine->isStopping) 
    {
        ofLogVerbose(__func__) << "Exit signal detected, waiting for next key frame boundry before exiting...";
        engine->isStopping = true;
        isKeyframeValid = encoderOutputBuffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME;
    }
    if(engine->isStopping && (isKeyframeValid ^ (encoderOutputBuffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME))) 
    {
        ofLogVerbose(__func__) << "Key frame boundry reached, exiting loop...";
        engine->writeFile();
    }else 
    {
        engine->recordingFileBuffer.append((const char*) encoderOutputBuffer->pBuffer + encoderOutputBuffer->nOffset, encoderOutputBuffer->nFilledLen);
        //ofLogVerbose(__func__) << "encoderOutputBuffer->nFilledLen: " << encoderOutputBuffer->nFilledLen;
        ofLog() << engine->recordingFileBuffer.size();
        OMX_ERRORTYPE error = OMX_FillThisBuffer(encoder, encoderOutputBuffer);
        if(error != OMX_ErrorNone) 
        {
            ofLog(OF_LOG_ERROR, "encoder OMX_FillThisBuffer FAIL error: 0x%08x", error);
            if(!engine->didWriteFile)
            {
                ofLogError() << "HAD ERROR FILLING BUFFER, JUST WRITING WHAT WE HAVE";
                engine->writeFile();

            }
        }
    }
    return OMX_ErrorNone;
}

void VideoEngine::createEncoder()
{
#pragma mark ENCODER SETUP  
    
    OMX_ERRORTYPE error;
    
    OMX_CALLBACKTYPE encoderCallbacks;
    encoderCallbacks.EventHandler       = &VideoEngine::encoderEventHandlerCallback;
    encoderCallbacks.EmptyBufferDone    = &VideoEngine::nullEmptyBufferDone;
    encoderCallbacks.FillBufferDone     = &VideoEngine::encoderFillBufferDone;
    
    error =OMX_GetHandle(&encoder, OMX_VIDEO_ENCODER, this , &encoderCallbacks);
    OMX_TRACE(error);
    
    error = DisableAllPortsForComponent(&encoder);
    OMX_TRACE(error);
    
    // Encoder input port definition is done automatically upon tunneling
    
    
    
    OMX_PARAM_PORTDEFINITIONTYPE encoderOutputPortDefinition;
    OMX_INIT_STRUCTURE(encoderOutputPortDefinition);
    encoderOutputPortDefinition.nPortIndex = VIDEO_ENCODE_OUTPUT_PORT;
    error =OMX_GetParameter(encoder, OMX_IndexParamPortDefinition, &encoderOutputPortDefinition);
    OMX_TRACE(error);
    
    
    int recordingBitRate = MEGABYTE_IN_BITS * settings->recordingBitrateMB;
    
    
    encoderOutputPortDefinition.format.video.nBitrate = recordingBitRate;
    error = OMX_SetParameter(encoder, OMX_IndexParamPortDefinition, &encoderOutputPortDefinition);
    OMX_TRACE(error);
    
    // Configure encoding bitrate
    OMX_VIDEO_PARAM_BITRATETYPE encodingBitrate;
    OMX_INIT_STRUCTURE(encodingBitrate);
    encodingBitrate.eControlRate = OMX_Video_ControlRateVariable;
    //encodingBitrate.eControlRate = OMX_Video_ControlRateConstant;
    
    encodingBitrate.nTargetBitrate = recordingBitRate;
    encodingBitrate.nPortIndex = VIDEO_ENCODE_OUTPUT_PORT;
    
    error = OMX_SetParameter(encoder, OMX_IndexParamVideoBitrate, &encodingBitrate);
    OMX_TRACE(error);
    
    // Configure encoding format
    OMX_VIDEO_PARAM_PORTFORMATTYPE encodingFormat;
    OMX_INIT_STRUCTURE(encodingFormat);
    encodingFormat.nPortIndex = VIDEO_ENCODE_OUTPUT_PORT;
    encodingFormat.eCompressionFormat = OMX_VIDEO_CodingAVC;
    
    error = OMX_SetParameter(encoder, OMX_IndexParamVideoPortFormat, &encodingFormat);
    OMX_TRACE(error);
    
    error = OMX_GetParameter(encoder, OMX_IndexParamVideoPortFormat, &encodingFormat);
    OMX_TRACE(error);
    
    //Set encoder to Idle
    error = SetComponentState(encoder, OMX_StateIdle);
    OMX_TRACE(error);
    
    
    //Set splitter to Idle
    error = SetComponentState(splitter, OMX_StateIdle);
    OMX_TRACE(error);
    
    // Create splitter->encoder Tunnel
    error = OMX_SetupTunnel(splitter, VIDEO_SPLITTER_OUTPUT_PORT2,
                            encoder, VIDEO_ENCODE_INPUT_PORT);
    
    OMX_TRACE(error);
    //Enable splitter output2 port
    error = EnableComponentPort(splitter, VIDEO_SPLITTER_OUTPUT_PORT2);
    OMX_TRACE(error);
    
    //Enable encoder input port
    
    error = EnableComponentPort(encoder, VIDEO_ENCODE_INPUT_PORT);
    OMX_TRACE(error);
    
    
    //Enable encoder output port
    error = EnableComponentPort(encoder, VIDEO_ENCODE_OUTPUT_PORT);
    OMX_TRACE(error);
    
#pragma mark ENCODER BUFFERS SETUP  
    
    
    //OMX_PARAM_PORTDEFINITIONTYPE encoderOutputPortDefinition;
    //OMX_INIT_STRUCTURE(encoderOutputPortDefinition);
    //encoderOutputPortDefinition.nPortIndex = VIDEO_ENCODE_OUTPUT_PORT;
    error =OMX_GetParameter(encoder, OMX_IndexParamPortDefinition, &encoderOutputPortDefinition);
    OMX_TRACE(error);
    
    error =  OMX_AllocateBuffer(encoder, 
                                &encoderOutputBuffer, 
                                VIDEO_ENCODE_OUTPUT_PORT, 
                                NULL, 
                                encoderOutputPortDefinition.nBufferSize);
    
    
    OMX_TRACE(error);
    if(error != OMX_ErrorNone)
    {
        ofLogError(__func__) << "UNABLE TO RECORD - MAY REQUIRE MORE GPU MEMORY";
    }
    TRACE_LINE
    
}
void VideoEngine::destroyEncoder()
{
    if(!encoder) return;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    error = DisableAllPortsForComponent(&encoder);
    OMX_TRACE(error);
    
    error = OMX_SendCommand(encoder, OMX_CommandFlush, OMX_ALL, NULL);
    OMX_TRACE(error);
    
    error = OMX_FreeBuffer(encoder, VIDEO_ENCODE_OUTPUT_PORT, encoderOutputBuffer);
    OMX_TRACE(error);
    
    
    error = DisableComponentPort(splitter, VIDEO_SPLITTER_OUTPUT_PORT2);
    OMX_TRACE(error);
    
    /*
    error = OMX_SetupTunnel(splitter, VIDEO_SPLITTER_OUTPUT_PORT2,
                            NULL, 0);*/
    OMX_TRACE(error);
    
    error = OMX_SetupTunnel(encoder, VIDEO_ENCODE_INPUT_PORT,
                            NULL, 0);
    OMX_TRACE(error);
    
    error = OMX_FreeHandle(encoder);
    OMX_TRACE(error);
    encoder = NULL;
    
}


void VideoEngine::startRecording()
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    isRecording = true;
    
    createEncoder();
    
    error = SetComponentState(splitter, OMX_StateExecuting);
    OMX_TRACE(error);
    
    
    //Start encoder
    error = SetComponentState(encoder, OMX_StateExecuting);
    OMX_TRACE(error);
    
    error = OMX_FillThisBuffer(encoder, encoderOutputBuffer);
    OMX_TRACE(error);
}


void VideoEngine::stopRecording()
{
	stopRequested = true;
}

void VideoEngine::writeFile()
{
    
    OMX_ERRORTYPE error = OMX_ErrorNone;

    //stop encoder
    error = SetComponentState(encoder, OMX_StateIdle);
    OMX_TRACE(error);
    
    
	//format is raw H264 NAL Units
	ofLogVerbose(__func__) << "START";

	string filePath;
	
	if (settings->recordingFilePath == "") 
	{
        stringstream fileName;
        fileName << ofGetTimestampString() << "_";
        
        fileName << settings->sensorWidth << "x";
        fileName << settings->sensorHeight << "_";
        fileName << settings->framerate << "fps_";
        
        fileName << settings->recordingBitrateMB << "MBps_";
        
        fileName << recordedFrameCounter << "numFrames";
        
        fileName << ".h264";
		filePath = ofToDataPath(fileName.str(), true);
	}else
	{
		filePath = settings->recordingFilePath;
	}
	
	didWriteFile = ofBufferToFile(filePath, recordingFileBuffer, true);
	if(didWriteFile)
	{
		ofLogVerbose(__func__) << filePath  << " WRITE PASS";
        if(listener)
        {
            listener->onRecordingComplete(filePath);
        }
	}
    else
	{
		ofLogVerbose(__func__) << filePath << " FAIL";
	}
    
    recordingFileBuffer.clear();
    
    error = WaitForState(splitter, OMX_StateIdle);
    OMX_TRACE(error);
    
    destroyEncoder();
    
    error = WaitForState(splitter, OMX_StateExecuting);
    OMX_TRACE(error);
    
    isRecording = false;
    recordedFrameCounter = 0;
    stopRequested = false;
    isStopping = false;
    
}



void VideoEngine::close()
{
    if(!isOpen) return;
    isOpen = false;
    
    OMX_ERRORTYPE error = OMX_ErrorNone;

    OMX_CONFIG_PORTBOOLEANTYPE cameraport;
    OMX_INIT_STRUCTURE(cameraport);
    cameraport.nPortIndex = CAMERA_OUTPUT_PORT;
    cameraport.bEnabled = OMX_FALSE;
    
    error =OMX_SetParameter(camera, OMX_IndexConfigPortCapturing, &cameraport);    
    OMX_TRACE(error);
    
    
    error = DisableAllPortsForComponent(&camera);
    OMX_TRACE(error);
    
    if(settings->enableExtraVideoFilter)
    {
        error = DisableAllPortsForComponent(&imageFX);
        OMX_TRACE(error);
        
        OMX_PARAM_U32TYPE extra_buffers;
        OMX_INIT_STRUCTURE(extra_buffers);
        extra_buffers.nU32 = 0;
        
        error = OMX_SetParameter(imageFX, OMX_IndexParamBrcmExtraBuffers, &extra_buffers);
        OMX_TRACE(error);
    }
    
    error = DisableAllPortsForComponent(&splitter);
    OMX_TRACE(error);
    
  
    
    error = DisableAllPortsForComponent(&render);
    OMX_TRACE(error);
    
    destroyEncoder();
    


    

    error = OMX_SendCommand(camera, OMX_CommandFlush, OMX_ALL, NULL);
    OMX_TRACE(error);
    
    
    if(settings->enableExtraVideoFilter)
    {
        error = OMX_SendCommand(imageFX, OMX_CommandFlush, OMX_ALL, NULL);
        OMX_TRACE(error);
    }
    

    
    error = OMX_SendCommand(splitter, OMX_CommandFlush, OMX_ALL, NULL);
    OMX_TRACE(error);
    
    error = OMX_SendCommand(render, OMX_CommandFlush, OMX_ALL, NULL);
    OMX_TRACE(error);
    
    //Create splitter->render Tunnel
    error = OMX_SetupTunnel(splitter, VIDEO_SPLITTER_OUTPUT_PORT1,
                            NULL, 0);
    OMX_TRACE(error);
    
    
    error = OMX_SetupTunnel(camera, CAMERA_OUTPUT_PORT,
                            NULL, 0);
    OMX_TRACE(error);
    
    if(settings->enableExtraVideoFilter)
    {
        error = OMX_SetupTunnel(imageFX, IMAGE_FX_INPUT_PORT,
                                NULL, 0);
        
        error = OMX_SetupTunnel(imageFX, IMAGE_FX_OUTPUT_PORT,
                                NULL, 0);
    }

    error = OMX_SetupTunnel(splitter, VIDEO_SPLITTER_INPUT_PORT,
                            NULL, 0);
    OMX_TRACE(error);
    
    error = OMX_SetupTunnel(splitter, VIDEO_SPLITTER_OUTPUT_PORT2,
                            NULL, 0);
    OMX_TRACE(error);
    
    

    
    
    error = OMX_SetupTunnel(render, renderInputPort,
                            NULL, 0);
    OMX_TRACE(error);
    
    error = OMX_FreeHandle(camera);
    OMX_TRACE(error);
    
    if(settings->enableExtraVideoFilter)
    {
        error = OMX_FreeHandle(imageFX);
        OMX_TRACE(error);
    }
    
    error = OMX_FreeHandle(splitter);
    OMX_TRACE(error);
    
    error = OMX_FreeHandle(render);
    OMX_TRACE(error);
    
 
    if(listener)
    {
        listener->onVideoEngineClose();
    }
    camera = NULL;
    imageFX = NULL;
    splitter = NULL;
    render = NULL;
    encoder = NULL;
    listener = NULL;
    settings = NULL;
    eglBuffer = NULL;
    encoderOutputBuffer = NULL;
    eglImage = NULL;
    ofLogVerbose(__func__) << " END";

}

VideoEngine::~VideoEngine()
{
    close();
}

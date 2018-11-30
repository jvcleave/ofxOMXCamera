#pragma once
#include "ofxOMXCameraSettings.h"


class VideoRecordingListener
{
public:
    virtual void onVideoRecordingComplete(string filePath)=0;
};

class VideoRecorder
{
public:
    
    VideoRecorder();
    void setup(ofxOMXCameraSettings* settings_, OMX_HANDLETYPE splitter_, VideoRecordingListener* listener_);
    void startRecording();
    void stopRecording();
    OMX_ERRORTYPE setRecordingBitrate(float recordingBitrateMB_);

    
protected:
    ofxOMXCameraSettings* settings;
    OMX_HANDLETYPE splitter;
    OMX_HANDLETYPE encoder;
    
    ofBuffer recordingFileBuffer;
    OMX_BUFFERHEADERTYPE* encoderOutputBuffer;
    bool didWriteFile;
    int recordedFrameCounter;
    
    VideoRecordingListener* listener;
    bool stopRequested;
    bool isStopping;
    bool isRecording;

    void writeFile();
    void createEncoder();
    void destroyEncoder();
    static OMX_ERRORTYPE encoderEventHandlerCallback(OMX_HANDLETYPE camera, OMX_PTR videoModeEngine_, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
    static OMX_ERRORTYPE encoderFillBufferDone(OMX_HANDLETYPE encoder, OMX_PTR engine, OMX_BUFFERHEADERTYPE* encoderOutputBuffer);
    static OMX_ERRORTYPE nullEmptyBufferDone(OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE*){return OMX_ErrorNone;};


    
};

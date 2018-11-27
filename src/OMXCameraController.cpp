#include "OMXCameraController.h"

int zoomStepsSource[61] = 
{
    65536,  68157,  70124,  72745,
    75366,  77988,  80609,  83231,
    86508,  89784,  92406,  95683,
    99615,  102892, 106168, 110100,
    114033, 117965, 122552, 126484,
    131072, 135660, 140247, 145490,
    150733, 155976, 161219, 167117,
    173015, 178913, 185467, 192020,
    198574, 205783, 212992, 220201,
    228065, 236585, 244449, 252969,
    262144, 271319, 281149, 290980,
    300810, 311951, 322437, 334234,
    346030, 357827, 370934, 384041,
    397148, 411566, 425984, 441057,
    456131, 472515, 488899, 506593,
    524288
};
int isoEntries[10]{ 0, 100, 160, 200, 250,
                  320, 400, 500, 640, 800};
OMXCameraController::OMXCameraController()
{
    camera = NULL;
    pixelsRequested = false;

    //Requires gpio program provided via wiringPi
    //sudo apt-get install wiringpi
    ofFile gpioProgram("/usr/bin/gpio");
    hasGPIOProgram = gpioProgram.exists();
    if(zoomLevels.empty())
    {
        vector<int> converted(zoomStepsSource, 
                              zoomStepsSource + sizeof zoomStepsSource / sizeof zoomStepsSource[0]);
        zoomLevels = converted;
        
    }

    if(isoLevels.empty())
    {
        vector<int> converted(isoEntries, 
                              isoEntries + sizeof isoEntries / sizeof isoEntries[0]);
        isoLevels = converted;
        ofLog() << "isoLevels.size(): " << isoLevels.size();
    }
    
    resetValues();
}

void OMXCameraController::reset()
{
    resetValues();
    settings.resetValues();
    applyAllSettings();
}

void OMXCameraController::applyAllSettings()
{
    if(!camera)
    {
        ofLogError(__func__) << "NO CAMERA";
        
        return;
    }
    ofLogNotice(__func__) << settings.toString();
    setExposurePreset(settings.exposurePreset); 
    setMeteringType(settings.meteringType);
    setAutoISO(settings.autoISO);
    setISO(settings.ISO);    
    setAutoShutter(settings.autoShutter);
    setShutterSpeed(settings.shutterSpeed);
    
    setSharpness(settings.sharpness);
    setContrast(settings.contrast);
    setBrightness(settings.brightness);
    setSaturation(settings.saturation);
    setFrameStabilization(settings.frameStabilization);
    setWhiteBalance(settings.whiteBalance);
    setWhiteBalanceGainR(settings.whiteBalanceGainR);
    setWhiteBalanceGainB(settings.whiteBalanceGainB);

    if(settings.imageFilterParamConfig.name == settings.imageFilter)
    {
        vector<int> params = settings.imageFilterParamConfig.getParams();
        if(!params.empty())
        {
            setImageFilter(GetImageFilter(settings.imageFilter), params);

        }
    }else
    {
        setImageFilter(settings.imageFilter);
    }
    
    if(settings.enableExtraVideoFilter)
    {
        if(settings.extraImageFilterParamConfig.name == settings.extraImageFilter)
        {
            vector<int> params = settings.extraImageFilterParamConfig.getParams();
            if(!params.empty())
            {
                setExtraImageFilter(GetImageFilter(settings.extraImageFilter), params);
            }
        }else
        {
            setImageFilter(settings.extraImageFilter);
        }
    }
    
    
    
    setDRE(settings.dreLevel);
    setSensorCrop(settings.sensorCropRectangle);
    setZoomLevelNormalized(settings.zoomLevel);
    setRotation(settings.rotation);
    setMirror(settings.mirror);
    
    setSoftwareSharpening(settings.doDisableSoftwareSharpen);
    setSoftwareSaturation(settings.doDisableSoftwareSaturation);
    applyExposure();
    if(settings.burstModeEnabled)
    {
        //very specific so likely to fail so not checking unless really trying
        setBurstMode(settings.burstModeEnabled);

    }



    
    if(hasGPIOProgram)
    {
        settings.LED_PIN = getLEDPin();
        
        stringstream command;
        command << "gpio export ";
        command << settings.LED_PIN;
        command << " out";
        
        if(system(command.str().c_str()) == 0)
        {
            //silence compiler warning 
        }
        setLEDState(settings.LED);
    } 
    ofLogNotice(__func__)  << "END";
}

int OMXCameraController::getLEDPin()
{
    //default as RPI1 GPIO Layout
    int result = 5;
    if(hasGPIOProgram)
    {
        string command = "gpio -v";
        FILE* myPipe = popen(command.c_str(), "r");
        char buffer[128];
        string commandOutput = "";
        while (!feof(myPipe)) 
        {
            if (fgets(buffer, 128, myPipe) != NULL)
            {
                commandOutput += buffer;
            }
        } 
        
        pclose (myPipe);
        //ofLogVerbose(__func__) << "result: " << result;
        if(!commandOutput.empty())
        {
            vector<string> contents = ofSplitString(commandOutput, "Type:");
            string secondLine = contents[1];
            //ofLogVerbose(__func__) << "secondLine: " << secondLine;
            vector<string> secondLineContents = ofSplitString(secondLine, ",");
            string modelValue = secondLineContents[0];
            ofLogNotice(__func__) << "modelValue: " << modelValue;
            //assuming RPI2 and RPI3 GPIO layout is the same
            //TODO: check RPI3
            if(ofIsStringInString(modelValue, "2"))
            {
                result = 32;
            }
            if(ofIsStringInString(modelValue, "3"))
            {
                result = 134;
            }
        }
    }
    
    return result;
    
}

void OMXCameraController::resetValues()
{
    OMX_INIT_STRUCTURE(exposurePresetConfig);
    exposurePresetConfig.nPortIndex = OMX_ALL;
    
    OMX_INIT_STRUCTURE(sharpnessConfig);
    sharpnessConfig.nPortIndex = OMX_ALL;
    
    OMX_INIT_STRUCTURE(framestabilizationConfig);
    framestabilizationConfig.nPortIndex = OMX_ALL;
    
    OMX_INIT_STRUCTURE(contrastConfig);
    contrastConfig.nPortIndex = OMX_ALL;
    
    OMX_INIT_STRUCTURE(brightnessConfig);
    brightnessConfig.nPortIndex = OMX_ALL;
    
    OMX_INIT_STRUCTURE(saturationConfig);
    saturationConfig.nPortIndex = OMX_ALL;
    
    OMX_INIT_STRUCTURE(whiteBalanceConfig);
    whiteBalanceConfig.nPortIndex = OMX_ALL;
    
    OMX_INIT_STRUCTURE(sensorCropConfig);
    sensorCropConfig.nPortIndex = OMX_ALL;
    
    OMX_INIT_STRUCTURE(digitalZoomConfig);
    digitalZoomConfig.nPortIndex = OMX_ALL;
    
    OMX_INIT_STRUCTURE(rotationConfig);
    rotationConfig.nPortIndex = CAMERA_OUTPUT_PORT;
    
    OMX_INIT_STRUCTURE(mirrorConfig);
    mirrorConfig.nPortIndex = CAMERA_OUTPUT_PORT;
    
    OMX_INIT_STRUCTURE(disableSoftwareSharpenConfig);
    OMX_INIT_STRUCTURE(disableSoftwareSaturationConfig);
    
    OMX_INIT_STRUCTURE(dreConfig);
    
    OMX_INIT_STRUCTURE(exposureConfig);
    exposureConfig.nPortIndex = OMX_ALL;
    
    OMX_INIT_STRUCTURE(flickerCancelConfig);
    flickerCancelConfig.nPortIndex = OMX_ALL;
    
    OMX_INIT_STRUCTURE(burstModeConfig);  
    
    
    OMX_INIT_STRUCTURE(whiteBalanceGainsConfig);
    

    ofLogNotice(__func__) << endl;
}


#pragma mark IMAGE ENHANCEMENT


OMX_ERRORTYPE OMXCameraController::setSoftwareSaturation(bool state)
{
    if(!camera) return OMX_ErrorNone;
    
    OMX_ERRORTYPE error = OMX_GetConfig(camera, OMX_IndexParamSWSaturationDisable, &disableSoftwareSaturationConfig);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone) 
    {
        disableSoftwareSaturationConfig.bEnabled = toOMXBool(state);
        
        error = OMX_SetConfig(camera, OMX_IndexParamSWSaturationDisable, &disableSoftwareSaturationConfig);
        OMX_TRACE(error); 
        if(error == OMX_ErrorNone)
        {
            settings.doDisableSoftwareSaturation = state;
            ofLogVerbose() << "doDisableSoftwareSaturation: " << settings.doDisableSoftwareSaturation;
            
        }
    }
    
    return error;
}

OMX_ERRORTYPE OMXCameraController::enableSoftwareSaturation()
{
    return setSoftwareSaturation(false);
}
OMX_ERRORTYPE OMXCameraController::disableSoftwareSaturation()
{
    
    return setSoftwareSaturation(true);
}

OMX_ERRORTYPE OMXCameraController::setSoftwareSharpening(bool state)
{
    if(!camera) return OMX_ErrorNone;

    OMX_ERRORTYPE error = OMX_GetConfig(camera, OMX_IndexParamSWSharpenDisable, &disableSoftwareSharpenConfig);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone) 
    {
        disableSoftwareSharpenConfig.bEnabled = toOMXBool(state);
        
        error = OMX_SetConfig(camera, OMX_IndexParamSWSharpenDisable, &disableSoftwareSharpenConfig);
        OMX_TRACE(error);   
        if(error == OMX_ErrorNone)
        {
            settings.doDisableSoftwareSharpen = state;
            ofLogVerbose() << "doDisableSoftwareSharpen: " << settings.doDisableSoftwareSharpen;
        }
    }
    return error;
}

OMX_ERRORTYPE OMXCameraController::enableSoftwareSharpening()
{
    return setSoftwareSharpening(false);
}

OMX_ERRORTYPE OMXCameraController::disableSoftwareSharpening()
{
    
    return setSoftwareSharpening(true);
}

void OMXCameraController::setSharpness(int sharpness_) //-100 to 100
{
    
    if(!camera) return;

    settings.sharpness = sharpness_;
    
    OMX_ERRORTYPE error = OMX_ErrorNone;
    sharpnessConfig.nSharpness = settings.sharpness; 
    
    error = OMX_SetConfig(camera, OMX_IndexConfigCommonSharpness, &sharpnessConfig);
    OMX_TRACE(error);
    
}

void OMXCameraController::setContrast(int contrast_ ) //-100 to 100 
{
    
    if(!camera) return;
    
    settings.contrast = contrast_;
    
    OMX_ERRORTYPE error = OMX_ErrorNone;
    contrastConfig.nContrast = settings.contrast; 
    
    error = OMX_SetConfig(camera, OMX_IndexConfigCommonContrast, &contrastConfig);
    OMX_TRACE(error);
    
}

void OMXCameraController::setBrightness(int brightness_ ) //0 to 100
{
    if(!camera) return;
    
    settings.brightness = brightness_;
    
    OMX_ERRORTYPE error = OMX_ErrorNone;
    brightnessConfig.nBrightness = settings.brightness;
    
    error = OMX_SetConfig(camera, OMX_IndexConfigCommonBrightness, &brightnessConfig);
    OMX_TRACE(error);
    
}

void OMXCameraController::setSaturation(int saturation_) //-100 to 100
{
    
    if(!camera) return;
    
    settings.saturation = saturation_;
    //ofLogNotice(__func__) << "settings.saturation: " << settings.saturation;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    saturationConfig.nSaturation    = settings.saturation; 
    
    error = OMX_SetConfig(camera, OMX_IndexConfigCommonSaturation, &saturationConfig);
    OMX_TRACE(error);
    
}



#pragma mark ISO

int OMXCameraController::getISO() 
{ 
    return exposureConfig.nSensitivity;
}

bool OMXCameraController::getAutoISO()
{
    return fromOMXBool(exposureConfig.bAutoSensitivity);
}

OMX_ERRORTYPE OMXCameraController::setAutoISO(bool doAutoISO)
{
    if(!camera) return OMX_ErrorNone;

    OMX_ERRORTYPE error = OMX_GetConfig(camera, OMX_IndexConfigCommonExposureValue, &exposureConfig);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone) 
    {
        if(doAutoISO)
        {
            exposureConfig.bAutoSensitivity = OMX_TRUE; 
        }else
        {
            exposureConfig.bAutoSensitivity = OMX_FALSE; 
        }
        error = applyExposure();
        OMX_TRACE(error);
        
    }
    return error;
}


OMX_ERRORTYPE OMXCameraController::setISO(int ISO_)
{
    if(!camera) return OMX_ErrorNone;
    if(ISO_ < 0 || ISO_ > 800) 
    {
        ofLogError(__func__) << "ISO_: " << ISO_;
        return OMX_ErrorBadParameter;
    }
    
    OMX_ERRORTYPE error = OMX_GetConfig(camera, OMX_IndexConfigCommonExposureValue, &exposureConfig);
    OMX_TRACE(error);
    
    exposureConfig.nSensitivity = ISO_;
    error =  applyExposure();
    OMX_TRACE(error);
    
    if(error == OMX_ErrorNone)
    {
        settings.ISO = exposureConfig.nSensitivity;
        settings.isoNormalized = getISOLevelNormalized();
    }
    return error;
}

OMX_ERRORTYPE OMXCameraController::setISONormalized(float value)
{
    if(value<0 || value>1)
    {
        ofLogError(__func__) << value << "MUST BE BETWEEN 0.0 - 1.0";
        return OMX_ErrorBadParameter;
    }
    int isoIndex = (int) ofMap(value, 0.0f, 1.0f, 0, isoLevels.size()-1);
    
    OMX_ERRORTYPE error =  setISO(isoLevels[isoIndex]);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone)
    {
        settings.isoNormalized = value;

    }
    return error;
}

float OMXCameraController::getISOLevelNormalized()
{
    int low = isoLevels[0];
    int high = isoLevels.back();
    return ofNormalize(settings.ISO, low, high);
}

#pragma mark SHUTTER

bool OMXCameraController::getAutoShutter()
{
    return fromOMXBool(exposureConfig.bAutoShutterSpeed);
}

int OMXCameraController::getShutterSpeed()
{
    return exposureConfig.nShutterSpeedMsec;
}

OMX_ERRORTYPE OMXCameraController::setShutterSpeed(int shutterSpeedMicroSeconds_)
{
    if(!camera) return OMX_ErrorNone;
    OMX_ERRORTYPE error;
    
    if(shutterSpeedMicroSeconds_ >= 6000000)
    {
        OMX_PARAM_BRCMFRAMERATERANGETYPE fpsRangeConfig;
        OMX_INIT_STRUCTURE(fpsRangeConfig);
        fpsRangeConfig.nPortIndex = cameraOutputPort;
        
        error = OMX_GetConfig(camera, OMX_IndexParamBrcmFpsRange, &fpsRangeConfig);
        //167/1000 = 0.167, or approx 1/6 frames/sec = 6 secs/frame. 
        ofLog() << " PRE xFramerateLow: " << fromQ16(fpsRangeConfig.xFramerateLow);
        ofLog() << "PRE xFramerateHigh: " << fromQ16(fpsRangeConfig.xFramerateHigh);
        
        OMX_TRACE(error);
        fpsRangeConfig.xFramerateLow = toQ16(0.05);
        fpsRangeConfig.xFramerateHigh = toQ16(0.167);
        
        error = OMX_SetConfig(camera, OMX_IndexParamBrcmFpsRange, &fpsRangeConfig);
        OMX_TRACE(error); 
        
        
        error = OMX_GetConfig(camera, OMX_IndexParamBrcmFpsRange, &fpsRangeConfig);
        //167/1000 = 0.167, or approx 1/6 frames/sec = 6 secs/frame. 
        ofLog() << "POST xFramerateLow: " << fromQ16(fpsRangeConfig.xFramerateLow);
        ofLog() << "POST xFramerateHigh: " << fromQ16(fpsRangeConfig.xFramerateHigh);
        
    }
    
    error = OMX_GetConfig(camera, OMX_IndexConfigCommonExposureValue, &exposureConfig);
    OMX_TRACE(error);
    exposureConfig.nShutterSpeedMsec = shutterSpeedMicroSeconds_;
    error =  applyExposure();
    OMX_TRACE(error);
    if(error == OMX_ErrorNone)
    {
        settings.shutterSpeed = exposureConfig.nShutterSpeedMsec;
        settings.shutterSpeedNormalized = getShutterSpeedNormalized();
    }
    ofLogVerbose(__func__) << "POST getShutterSpeed(): " << getShutterSpeed();

    return error;

    
}

OMX_ERRORTYPE OMXCameraController::setShutterSpeedNormalized(float value)
{
    if(!camera) return OMX_ErrorNone;
    if(value<0 || value>1)
    {
        ofLogError(__func__) << value << "MUST BE BETWEEN 0.0 - 1.0";
        return OMX_ErrorBadParameter;
    }
    
    float microseconds = ofMap(value, 0.0, 1.0, 0.0f, 6000000);
    OMX_ERRORTYPE error = setShutterSpeed(microseconds);
    OMX_TRACE(error);
    return error;
    
}
float OMXCameraController::getShutterSpeedNormalized()
{
    int value = getShutterSpeed();
    float result = ofMap(value, 0, 6000000, 0.0f, 1.0f);
    return result;
}
OMX_ERRORTYPE OMXCameraController::setAutoShutter(bool doAutoShutter)
{
    if(!camera) return OMX_ErrorNone;

    OMX_ERRORTYPE error = OMX_GetConfig(camera, OMX_IndexConfigCommonExposureValue, &exposureConfig);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone) 
    {
        
        if(doAutoShutter)
        {
            exposureConfig.bAutoShutterSpeed = OMX_TRUE; 
        }else
        {
            exposureConfig.bAutoShutterSpeed = OMX_FALSE; 
        }
        error =  applyExposure();
        
        if(error == OMX_ErrorNone)
        {
            settings.autoShutter = getAutoShutter();
        }
    }
    return error;
}

void OMXCameraController::setFrameStabilization(bool doStabilization)
{
    if(!camera) return;
    
    OMX_ERRORTYPE error = OMX_ErrorNone;
    if (doStabilization) 
    {
        framestabilizationConfig.bStab = OMX_TRUE;
    }else 
    {
        framestabilizationConfig.bStab = OMX_FALSE;
    }
    
    error = OMX_SetConfig(camera, OMX_IndexConfigCommonFrameStabilisation, &framestabilizationConfig);
    if(error == OMX_ErrorNone) 
    {
        settings.frameStabilization = doStabilization;
    }
    
    OMX_TRACE(error);
    
}

void OMXCameraController::checkBurstMode()
{
    if(!camera) return;
    
    OMX_ERRORTYPE error = OMX_GetConfig(camera, OMX_IndexConfigBurstCapture, &burstModeConfig);
    OMX_TRACE(error);
    settings.burstModeEnabled = fromOMXBool(burstModeConfig.bEnabled);
}

void OMXCameraController::setBurstMode(bool doBurstMode)
{
    if(!camera) return;

    OMX_ERRORTYPE error = OMX_GetConfig(camera, OMX_IndexConfigBurstCapture, &burstModeConfig);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone)
    {
        if(doBurstMode)
        {
            burstModeConfig.bEnabled = OMX_TRUE;  
        }else
        {
            burstModeConfig.bEnabled = OMX_FALSE;  
            
        }
        error = OMX_SetConfig(camera, OMX_IndexConfigBurstCapture, &burstModeConfig);
        OMX_TRACE(error);
        if(error == OMX_ErrorNone)
        {
            settings.burstModeEnabled = doBurstMode;
        }
        ofLogVerbose() << "burstModeEnabled: " << settings.burstModeEnabled;
    }
    
}

#pragma mark FLICKER CANCELLATION

void OMXCameraController::checkFlickerCancellation()
{
    if(!camera) return;
    
    OMX_ERRORTYPE error = OMX_GetConfig(camera, OMX_IndexConfigCommonFlickerCancellation, &flickerCancelConfig);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone) 
    {
        switch (flickerCancelConfig.eFlickerCancel) 
        {
            case OMX_COMMONFLICKERCANCEL_OFF:
            {
                ofLogVerbose(__func__) << "OMX_COMMONFLICKERCANCEL_OFF";
                break;
            }
            case OMX_COMMONFLICKERCANCEL_AUTO:
            {
                ofLogVerbose(__func__) << "OMX_COMMONFLICKERCANCEL_AUTO";
                break;
            }
            case OMX_COMMONFLICKERCANCEL_50:
            {
                ofLogVerbose(__func__) << "OMX_COMMONFLICKERCANCEL_50";
                break;
            }
            case OMX_COMMONFLICKERCANCEL_60:
            {
                ofLogVerbose(__func__) << "OMX_COMMONFLICKERCANCEL_60";
                break;
            }
            default:
                break;
        }
        if(flickerCancelConfig.eFlickerCancel == OMX_COMMONFLICKERCANCEL_OFF)
        {
            settings.flickerCancellation = false;
        }else
        {
            settings.flickerCancellation = true;
        }
    }
    else
    {
        //error so assume it's not working
        settings.flickerCancellation = false;
    }
}
void OMXCameraController::setFlickerCancellation(bool enable)
{
    if(enable)
    {
        setFlickerCancellation(OMX_COMMONFLICKERCANCEL_AUTO); 
    }else
    {
        setFlickerCancellation(OMX_COMMONFLICKERCANCEL_OFF); 
    }
    
}

void OMXCameraController::enableFlickerCancellation()
{
    setFlickerCancellation(true);
}

void OMXCameraController::disableFlickerCancellation()
{
    setFlickerCancellation(false);
}


OMX_ERRORTYPE OMXCameraController::setFlickerCancellation(OMX_COMMONFLICKERCANCELTYPE eFlickerCancel)
{
    if(!camera) return OMX_ErrorNone;

    OMX_ERRORTYPE error = OMX_GetConfig(camera, OMX_IndexConfigCommonFlickerCancellation, &flickerCancelConfig);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone) 
    {
        switch (flickerCancelConfig.eFlickerCancel) 
        {
            case OMX_COMMONFLICKERCANCEL_OFF:
            {
                ofLogVerbose(__func__) << "OMX_COMMONFLICKERCANCEL_OFF";
                break;
            }
            case OMX_COMMONFLICKERCANCEL_AUTO:
            {
                ofLogVerbose(__func__) << "OMX_COMMONFLICKERCANCEL_AUTO";
                break;
            }
            case OMX_COMMONFLICKERCANCEL_50:
            {
                ofLogVerbose(__func__) << "OMX_COMMONFLICKERCANCEL_50";
                break;
            }
            case OMX_COMMONFLICKERCANCEL_60:
            {
                ofLogVerbose(__func__) << "OMX_COMMONFLICKERCANCEL_60";
                break;
            }
            default:
                break;
        }
        flickerCancelConfig.eFlickerCancel = eFlickerCancel;
        error = OMX_SetConfig(camera, OMX_IndexConfigCommonFlickerCancellation, &flickerCancelConfig);
        if(error == OMX_ErrorNone)
        {
            if(flickerCancelConfig.eFlickerCancel == OMX_COMMONFLICKERCANCEL_OFF)
            {
                settings.flickerCancellation = false;
            }else
            {
                settings.flickerCancellation = true;
            }
        }else
        {
            //error so assume it's not working
            settings.flickerCancellation = false;
        }
        OMX_TRACE(error);
    }
    else
    {
        //error so assume it's not working
        settings.flickerCancellation = false;
    }
    
    return error;
}


#pragma mark DYNAMIC RANGE EXPANSION (DRE)

void OMXCameraController::setDRE(int level)
{
    OMX_DYNAMICRANGEEXPANSIONMODETYPE type = OMX_DynRangeExpOff;
    switch (level) 
    {
        case 0:
        {
            type = OMX_DynRangeExpOff;
            break;
        }
        case 1:
        {
            type = OMX_DynRangeExpLow;
            break;
        }
        case 2:
        {
            type = OMX_DynRangeExpMedium;
            break;
        }
        case 3:
        {
            type = OMX_DynRangeExpHigh;
            break;
        }
        default:
        {
            type = OMX_DynRangeExpOff;
            break;
        }
            
    }
    OMX_CONFIG_DYNAMICRANGEEXPANSIONTYPE dreConfig;
    OMX_INIT_STRUCTURE(dreConfig);
    dreConfig.eMode = type;
    OMX_ERRORTYPE error = OMX_SetConfig(camera, OMX_IndexConfigDynamicRangeExpansion, &dreConfig);
    OMX_TRACE(error);
    
    if(error == OMX_ErrorNone)
    {
        settings.dreLevel = level;
    }
}

#pragma mark METERING TYPE

OMX_ERRORTYPE OMXCameraController::setMeteringType(OMX_METERINGTYPE meteringType_)
{
    exposureConfig.eMetering = meteringType_;
    OMX_ERRORTYPE error = applyExposure();
    if(error == OMX_ErrorNone)
    {
        settings.meteringType     = getMeteringType();
    }
    
    return error;
}

string OMXCameraController::getMeteringType()
{
    return GetMeteringString(exposureConfig.eMetering);
}

OMX_ERRORTYPE OMXCameraController::setMeteringType(string meteringType_)
{    
    return setMeteringType(GetMetering(meteringType_));
}

OMX_ERRORTYPE OMXCameraController::applyExposure()
{
    if(!camera) return OMX_ErrorNone;

    OMX_ERRORTYPE error = OMX_ErrorNone;
    error = OMX_SetConfig(camera, OMX_IndexConfigCommonExposureValue, &exposureConfig);
    OMX_TRACE(error);
#if 0
    stringstream info;
    info << "eMetering: " << exposureConfig.eMetering << endl;
    info << "bAutoShutterSpeed: " << exposureConfig.xEVCompensation << endl;
    info << "xEVCompensation: " << exposureConfig.xEVCompensation << endl;
    info << "nShutterSpeedMsec: " << exposureConfig.nShutterSpeedMsec << endl;
    info << "bAutoSensitivity: " << exposureConfig.bAutoSensitivity << endl;
    info << "nSensitivity: " << exposureConfig.nSensitivity << endl;
    info << endl;
    info << "eMetering: " << GetMeteringString(exposureConfig.eMetering) << endl;
    info << "autoShutter: " << getAutoShutter() << endl;
    info << "shutterSpeed: " << getShutterSpeed() << endl;
    info << "autoISO: " << getAutoISO() << endl;
    info << "ISO: " << getISO() << endl;
    ofLogVerbose(__func__) << " info: " << info.str();
#endif
    /*if (error == OMX_ErrorNone) 
     {
     meteringType     = exposureConfig.eMetering;
     evCompensation   = fromQ16(exposureConfig.xEVCompensation);
     
     autoShutter      = fromOMXBool(exposureConfig.bAutoShutterSpeed);
     shutterSpeed     = exposureConfig.nShutterSpeedMsec;
     
     autoISO          = fromOMXBool(exposureConfig.bAutoSensitivity);
     ISO              = exposureConfig.nSensitivity;
     }*/
    //ofLogVerbose(__func__) << printExposure();
    
    return error;
}

OMX_ERRORTYPE OMXCameraController::setEvCompensation(int value)
{
    if ((value >= -4) && (value <= 4)) 
    {
        exposureConfig.xEVCompensation = toQ16(value);
    }
    OMX_ERRORTYPE error =  applyExposure();
    if(error == OMX_ErrorNone)
    {
        settings.evCompensation = getEvCompensation();
    }
    return error;
}


#pragma mark WHITE BALANCE

OMX_ERRORTYPE OMXCameraController::setWhiteBalance(OMX_WHITEBALCONTROLTYPE whiteBalance_)
{    
    if(!camera) return OMX_ErrorNone;
    ofLogNotice(__func__) << GetWhiteBalanceString(whiteBalance_);
    
    whiteBalanceConfig.eWhiteBalControl = whiteBalance_;
    
    OMX_ERRORTYPE error = OMX_SetConfig(camera, OMX_IndexConfigCommonWhiteBalance, &whiteBalanceConfig);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone)
    {
        settings.whiteBalance = GetWhiteBalanceString(whiteBalance_);
    }
    return error;
}

OMX_ERRORTYPE OMXCameraController::setWhiteBalanceGainR(float value)
{
    
    OMX_ERRORTYPE error;
    if (value < 0 || value > 1) 
    {
        return OMX_ErrorBadParameter;
    }

    whiteBalanceGainsConfig.xGainR = toQ16(value);
    
    error = OMX_SetConfig(camera, OMX_IndexConfigCustomAwbGains , &whiteBalanceGainsConfig );
    OMX_TRACE(error);
    if(error == OMX_ErrorNone)
    {
        settings.whiteBalanceGainR = value;
    }
    return error;
}


OMX_ERRORTYPE OMXCameraController::setWhiteBalanceGainB(float value)
{
    
    OMX_ERRORTYPE error;
    if (value < 0 || value > 1) 
    {
        return OMX_ErrorBadParameter;
    }

    whiteBalanceGainsConfig.xGainB = toQ16(value);
    
    error = OMX_SetConfig(camera, OMX_IndexConfigCustomAwbGains , &whiteBalanceGainsConfig );
    OMX_TRACE(error);
    if(error == OMX_ErrorNone)
    {
        settings.whiteBalanceGainB = value;
    }
    return error;
}


OMX_ERRORTYPE OMXCameraController::setWhiteBalance(string name)
{    
    
    return setWhiteBalance(GetWhiteBalance(name));
}

string OMXCameraController::getWhiteBalance()
{
    return GetWhiteBalanceString(whiteBalanceConfig.eWhiteBalControl);
}

#pragma mark CROP

OMX_ERRORTYPE OMXCameraController::updateSensorCrop()
{
    return setSensorCrop(settings.sensorCropRectangle);
}

OMX_ERRORTYPE OMXCameraController::setSensorCrop(int left, int top, int width, int height)
{
    settings.sensorCropRectangle.set(left, top, width, height);
    return updateSensorCrop();
}

OMX_ERRORTYPE OMXCameraController::setSensorCrop(ofRectangle& rectangle)
{
    
    if(!camera) return OMX_ErrorNone;

    sensorCropConfig.xLeft   = ((uint32_t)rectangle.getLeft()   << 16)/100;
    sensorCropConfig.xTop    = ((uint32_t)rectangle.getTop()    << 16)/100;
    sensorCropConfig.xWidth  = ((uint32_t)rectangle.getWidth()  << 16)/100;
    sensorCropConfig.xHeight = ((uint32_t)rectangle.getHeight() << 16)/100;
    
    OMX_ERRORTYPE error = OMX_SetConfig(camera, OMX_IndexConfigInputCropPercentages, &sensorCropConfig);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone)
    {
        ofLogError(__func__) << omxErrorToString(error);
        if(error == OMX_ErrorBadParameter)
        {
            ofLogWarning(__func__) << "resetting sensorCropRectangle to known good params (0, 0, 100, 100)";
            settings.sensorCropRectangle.set(0, 0, 100, 100);
            return updateSensorCrop(); 
        }
        
    }
    return error;
    
}

#pragma mark ZOOM

OMX_ERRORTYPE OMXCameraController::setZoomLevelNormalized(float value)
{
    if(value<0 || value>1)
    {
        ofLogError(__func__) << value << "MUST BE BETWEEN 0.0 - 1.0";
        return OMX_ErrorBadParameter;
    }
    settings.zoomLevel = (int) ofMap(value, 0.0f, 1.0f, 0, zoomLevels.size()-1);
    
    
    return setDigitalZoom();
}

OMX_ERRORTYPE OMXCameraController::resetZoom()
{
    settings.zoomLevel = 0;
    return setDigitalZoom();
}

OMX_ERRORTYPE OMXCameraController::zoomIn()
{
    if((unsigned int)settings.zoomLevel+1 < zoomLevels.size())
    {
        settings.zoomLevel++;
    }
    return setDigitalZoom();
}

OMX_ERRORTYPE OMXCameraController::zoomOut()
{
    settings.zoomLevel--;
    if(settings.zoomLevel<0)
    {
        settings.zoomLevel = 0;
    }
    
    return setDigitalZoom();
}

OMX_ERRORTYPE OMXCameraController::setDigitalZoom()
{
    if(!camera) return OMX_ErrorNone;

    if(settings.zoomLevel<0 || (unsigned int) settings.zoomLevel>zoomLevels.size())
    {
        
        ofLogError(__func__) << "BAD zoomLevel: " << settings.zoomLevel << " SETTING TO 0" << " zoomLevels.size(): " << zoomLevels.size();
        settings.zoomLevel = 0;
    }
    
    
    int value = zoomLevels[settings.zoomLevel];
    if(digitalZoomConfig.xWidth != value && digitalZoomConfig.xHeight != value)
    {
        digitalZoomConfig.xWidth  = value;
        digitalZoomConfig.xHeight = value;
        
        OMX_ERRORTYPE error = OMX_SetConfig(camera, OMX_IndexConfigCommonDigitalZoom, &digitalZoomConfig);
        OMX_TRACE(error);
        
        if(error == OMX_ErrorNone)
        {
            settings.zoomLevelNormalized = getZoomLevelNormalized();
        }
        return error;
    }
    return OMX_ErrorNone;
    
}

float OMXCameraController::getZoomLevelNormalized()
{
    return ofMap(settings.zoomLevel, 0, zoomLevels.size(), 0.0f, 1.0f);
}

#pragma mark MIRROR

string OMXCameraController::getMirror()
{
    
    return GetMirrorString(mirrorConfig.eMirror);    
}

OMX_ERRORTYPE OMXCameraController::setMirror(int mirrorType)
{
    mirrorConfig.eMirror = (OMX_MIRRORTYPE)mirrorType;
    return applyMirror();
}

OMX_ERRORTYPE OMXCameraController::setMirror(string mirror_)
{
    return setMirror(GetMirror(mirror_));
}

OMX_ERRORTYPE OMXCameraController::applyMirror()
{
    if(!camera) return OMX_ErrorNone;

    OMX_ERRORTYPE error = OMX_SetConfig(camera, OMX_IndexConfigCommonMirror, &mirrorConfig);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone)
    {
        settings.mirror = GetMirrorString(mirrorConfig.eMirror);
    }
    return error;
}

#pragma mark ROTATION

OMX_ERRORTYPE OMXCameraController::setRotation(int value)
{
    return setRotation((ROTATION) value);
}

OMX_ERRORTYPE OMXCameraController::setRotation(ROTATION value)
{
    switch (value)
    {
        case ROTATION_0: {rotationConfig.nRotation=0;} break;
        case ROTATION_90: {rotationConfig.nRotation=90;} break;
        case ROTATION_180: {rotationConfig.nRotation=180;} break;
        case ROTATION_270: {rotationConfig.nRotation=270;} break;
        default: {rotationConfig.nRotation=0;} break;
    }
    return applyRotation();
}

int OMXCameraController::getRotation()
{
    return rotationConfig.nRotation;
}

OMX_ERRORTYPE OMXCameraController::applyRotation()
{
    
    if(!camera) return OMX_ErrorNone;

    OMX_ERRORTYPE error = OMX_SetConfig(camera, OMX_IndexConfigCommonRotate, &rotationConfig);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone)
    {
        settings.rotation = getRotation();
    }
    return error;
}

OMX_ERRORTYPE OMXCameraController::rotateClockwise()
{
    int currentRotation  = getRotation();
    if(currentRotation+90<360)
    {
        rotationConfig.nRotation+=90;
    }else{
        rotationConfig.nRotation = 0;
    }
    
    return applyRotation();
}

OMX_ERRORTYPE OMXCameraController::rotateCounterClockwise()
{
    int currentRotation  = getRotation();
    if(currentRotation-90>=0)
    {
        rotationConfig.nRotation-=90;
    }else{
        rotationConfig.nRotation = 270;
    }
    
    return applyRotation();
}


#pragma mark FILTERS

OMX_ERRORTYPE OMXCameraController::setImageFilter(string imageFilter)
{
    return setImageFilter(GetImageFilter(imageFilter));
}

string OMXCameraController::getImageFilter()
{
    return settings.imageFilter;
}

OMX_ERRORTYPE OMXCameraController::setImageFilter(OMX_IMAGEFILTERTYPE imageFilter)
{
    if(!camera) return OMX_ErrorNone;
    
    OMX_ERRORTYPE error = imageFXController.setImageFilter(imageFilter);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone)
    {
        settings.imageFilter = GetImageFilterString(imageFilter);
    }
    return error;
    
}

OMX_ERRORTYPE OMXCameraController::setImageFilter(OMX_IMAGEFILTERTYPE imageFilter, vector<int>& params)
{
    OMX_ERRORTYPE error =  imageFXController.setImageFilter(imageFilter, params);
    if(error == OMX_ErrorNone)
    {
        settings.imageFilter = GetImageFilterString(imageFilter);
        settings.imageFilterParamConfig  = settings.getFilterParamConfig(imageFilter, params);
    }
    return error;
}

OMX_ERRORTYPE OMXCameraController::setExtraImageFilter(string imageFilter)
{
    OMX_ERRORTYPE error = OMX_ErrorNotReady;
    if(settings.enableExtraVideoFilter)
    {
        error = extraImageFXController.setImageFilter(imageFilter);
        if(error == OMX_ErrorNone)
        {
            settings.extraImageFilter = imageFilter;
        }
    }else
    {
        ofLogError(__func__) << "enableExtraVideoFilter IS FALSE";
    }
    return error;
}

OMX_ERRORTYPE OMXCameraController::setExtraImageFilter(OMX_IMAGEFILTERTYPE imageFilter, vector<int>& params)
{
    OMX_ERRORTYPE error = OMX_ErrorNotReady;
    
    if(settings.enableExtraVideoFilter)
    {
        
        error = extraImageFXController.setImageFilter(imageFilter, params);
        if(error == OMX_ErrorNone)
        {
            settings.extraImageFilter = GetImageFilterString(imageFilter);
            settings.extraImageFilterParamConfig  = settings.getFilterParamConfig(imageFilter, params);
        }
    }else
    {
        ofLogError(__func__) << "enableExtraVideoFilter IS FALSE";
    }
    return error;
    
}


OMX_ERRORTYPE OMXCameraController::setColorEnhancement(bool doEnhance, int u, int v)
{    
    return extraImageFXController.setColorEnhancement(doEnhance, u, v);
}


#pragma mark EXPOSURE PRESETS

OMX_ERRORTYPE OMXCameraController::setExposurePreset(OMX_EXPOSURECONTROLTYPE exposurePreset_)
{
    if(!camera) return OMX_ErrorNotReady;

    exposurePresetConfig.eExposureControl = exposurePreset_;
    
    OMX_ERRORTYPE error = OMX_SetConfig(camera, OMX_IndexConfigCommonExposure, &exposurePresetConfig);
    OMX_TRACE(error);
    if(error == OMX_ErrorNone)
    {
        settings.exposurePreset = GetExposurePresetString(exposurePreset_);
    }
    return error;
}

OMX_ERRORTYPE OMXCameraController::setExposurePreset(string exposurePreset_)
{
    
    return setExposurePreset(GetExposurePreset(exposurePreset_));
}


string OMXCameraController::getExposurePreset()
{
    return GetExposurePresetString(exposurePresetConfig.eExposureControl);
}

void OMXCameraController::toggleLED()
{
    setLEDState(!settings.LED);
}

void OMXCameraController::setLEDState(bool stateRequested)
{
    if (!hasGPIOProgram) 
    {
        return;
    }
    stringstream command;
    command << "gpio -g write ";
    command << settings.LED_PIN;
    if (stateRequested) 
    {
        command <<  " 1";
    }else
    {
        command <<  " 0";
    }
    
    
    int result = system(command.str().c_str());
    if(result == 0)
    {
        settings.LED = stateRequested;
    }
}





int OMXCameraController::getEvCompensation()
{
    return fromQ16(exposureConfig.xEVCompensation);
}

string OMXCameraController::printExposure()
{
    
    
    stringstream ss;
    ss << "meteringType: " << OMX_Maps::getInstance().getMetering(exposureConfig.eMetering) << "\n";
    ss << "evCompensation: " << getEvCompensation() << "\n";
    
    ss << "autoShutter: " << getAutoShutter() << "\n";
    ss << "shutterSpeedMicroSeconds: " << getShutterSpeed() << "\n";
    
    ss << "autoISO: " << getAutoISO() << "\n";
    ss << "ISO: " << getISO() << "\n";
    return ss.str();
    
}


void OMXCameraController::setDisplayAlpha(int alpha)
{
    displayController.setDisplayAlpha(alpha);
}

void OMXCameraController::setDisplayLayer(int layer)
{
    displayController.setDisplayLayer(layer);
}

void OMXCameraController::setDisplayRotation(int rotationDegrees)
{
    displayController.setDisplayRotation(rotationDegrees);
}

void OMXCameraController::setDisplayDrawRectangle(ofRectangle& drawRectangle)
{
    displayController.setDisplayDrawRectangle(drawRectangle);
}

void OMXCameraController::setDisplayCropRectangle(ofRectangle& drawCropRectangle)
{
    displayController.setDisplayCropRectangle(drawCropRectangle);
}

void OMXCameraController::setDisplayMirror(bool doMirror)
{
    displayController.setDisplayMirror(doMirror);
}


GLuint OMXCameraController::getTextureID()
{
    return displayController.textureID;
}

void OMXCameraController::enablePixels()
{
    if(settings.enableTexture)
    {
        pixelsRequested = true;
    }
}

void OMXCameraController::disablePixels()
{
    if(settings.enableTexture)
    {
        pixelsRequested = false;
    }
}

unsigned char * OMXCameraController::getRawPixels()
{
    return displayController.pixels;
}

ofPixels& OMXCameraController::getPixels()
{
    return displayController.of_pixels;
}

ofTexture& OMXCameraController::getTextureReference()
{
    return displayController.texture;
}


void OMXCameraController::setZeroShutterLag(bool value)
{
    OMX_CONFIG_ZEROSHUTTERLAGTYPE config;
    OMX_INIT_STRUCTURE(config);
    //requests that the camera should run preview using the full resolution sensor mode
    config.bZeroShutterMode = OMX_FALSE;
    //whether captures should then grab the last raw preview image and generate the capture based on that
    config.bConcurrentCapture = toOMXBool(value);

    OMX_ERRORTYPE error = OMX_SetConfig(camera, OMX_IndexParamCameraZeroShutterLag, &config);
    OMX_TRACE(error);

}

#pragma mark UNSUPPORTED
void OMXCameraController::enableAnnotation()
{
    OMX_CONFIG_BRCMANNOTATETYPE config;
    OMX_INIT_STRUCTURE(config);
    
    OMX_ERRORTYPE error = OMX_GetConfig(camera, OMX_IndexConfigBrcmCameraAnnotate, &config);
    OMX_TRACE(error);
    
    stringstream info;
    
    
    info << "config.bEnable: " << config.bEnable << endl;
    info << "bShowShutter: " << config.bShowShutter << endl;
    info << "bShowAnalogGain: " << config.bShowAnalogGain << endl;
    info << "bShowLens: " << config.bShowLens << endl;
    info << "bShowCaf: " << config.bShowCaf << endl;
    info << "bShowMotion: " << config.bShowMotion << endl;
    info << "bShowFrameNum: " << config.bShowFrameNum << endl;
    info << "bEnableBackground: " << config.bEnableBackground << endl;
    info << "bCustomBackgroundColour: " << config.bCustomBackgroundColour << endl;
    
    ofLogNotice(__func__) << info.str();
    config.bEnable = OMX_TRUE;
#if 0
    config.bShowShutter = OMX_TRUE;
    config.bShowAnalogGain = OMX_TRUE;
    config.bShowLens = OMX_TRUE;
    config.bShowCaf = OMX_TRUE;
    config.bShowMotion = OMX_TRUE;
    config.bShowFrameNum = OMX_TRUE;
    config.bEnableBackground = OMX_TRUE;
    config.bCustomBackgroundColour = OMX_FALSE;
    
    OMX_U8 nBackgroundY;
    OMX_U8 nBackgroundU;
    OMX_U8 nBackgroundV;
    OMX_U8 dummy1;
    OMX_BOOL bCustomTextColour;
    OMX_U8 nTextY;
    OMX_U8 nTextU;
    OMX_U8 nTextV;
    OMX_U8 nTextSize;
    OMX_U8 sText[OMX_BRCM_MAXANNOTATETEXTLEN];
    OMX_BRCMANNOTATEJUSTIFYTYPE eJustify;
    OMX_U32 nXOffset;
    OMX_U32 nYOffset;
#endif
    error = OMX_GetConfig(camera, OMX_IndexConfigBrcmCameraAnnotate, &config);
    OMX_TRACE(error);
    
}
OMX_ERRORTYPE OMXCameraController::setColorDenoise(bool doColorDenoise)
{
    
    if(!camera) return OMX_ErrorNone;
    OMX_CONFIG_BOOLEANTYPE colorDenoise;
    OMX_INIT_STRUCTURE(colorDenoise);
    colorDenoise.bEnabled = toOMXBool(doColorDenoise);
    OMX_ERRORTYPE error = OMX_SetConfig(camera, OMX_IndexConfigStillColourDenoiseEnable, &colorDenoise);
    OMX_TRACE(error);
    return error;
    
}


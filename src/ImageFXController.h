#pragma mark
#include "ofxOMXCameraSettings.h"


class ImageFXController
{
public:
    
    
    OMX_HANDLETYPE component;
    int componentPort;
    ImageFXController()
    {
        component = NULL;
        componentPort = OMX_ALL;

    }
    
   
    
    void setup(OMX_HANDLETYPE component_, int componentPort_)
    {
        component = component_;
        componentPort = componentPort_;
 
    }
    
    OMX_ERRORTYPE setImageFilter(string imageFilterName)
    {
        OMX_IMAGEFILTERTYPE imageFilter = GetImageFilter(imageFilterName);
        return setImageFilter(imageFilter);
    }
    
    
    OMX_ERRORTYPE setImageFilter(OMX_IMAGEFILTERTYPE imageFilter)
    {
        if(!component)
        {
            ofLogError(__func__) << "NO COMPONENT YET";
            return OMX_ErrorNotReady;
        }
        
        OMX_CONFIG_IMAGEFILTERTYPE imagefilterConfig;
        OMX_INIT_STRUCTURE(imagefilterConfig);
        imagefilterConfig.nPortIndex = componentPort;
        imagefilterConfig.eImageFilter = imageFilter;
        
        OMX_ERRORTYPE error = OMX_SetConfig(component, OMX_IndexConfigCommonImageFilter, &imagefilterConfig);
        OMX_TRACE(error); 
        return error;
    }
    
    OMX_ERRORTYPE setImageFilter(OMX_IMAGEFILTERTYPE imageFilter, vector<int> params)
    {
        if(!component)
        {
            ofLogError(__func__) << "NO COMPONENT YET";
            return OMX_ErrorNotReady;
        }
        
        OMX_ERRORTYPE error  = setImageFilter(imageFilter);
        OMX_TRACE(error);
        
        OMX_CONFIG_IMAGEFILTERPARAMSTYPE filtersParams;
        OMX_INIT_STRUCTURE(filtersParams);
        filtersParams.nPortIndex = componentPort;
        filtersParams.eImageFilter = imageFilter;
        filtersParams.nNumParams = params.size();
        for(int i=0; i<params.size(); i++)
        {
            filtersParams.nParams[i] = params[i];        
        }
        error =  OMX_SetConfig(component, OMX_IndexConfigCommonImageFilterParameters, &filtersParams);
        OMX_TRACE(error);
        return error;
    }
    
    
    OMX_ERRORTYPE setColorEnhancement(bool enable, int U, int V)
    {
        
        if(!component)
        {
            ofLogError(__func__) << "NO COMPONENT YET";
            return OMX_ErrorNotReady;
        }
        OMX_CONFIG_COLORENHANCEMENTTYPE colorEnhancementConfig;
        
        OMX_INIT_STRUCTURE(colorEnhancementConfig);
        colorEnhancementConfig.nPortIndex = componentPort;
        colorEnhancementConfig.bColorEnhancement = toOMXBool(enable);
        colorEnhancementConfig.nCustomizedU = U;
        colorEnhancementConfig.nCustomizedV = V;
        OMX_ERRORTYPE error = OMX_SetConfig(component, OMX_IndexConfigCommonColorEnhancement, &colorEnhancementConfig);
        OMX_TRACE(error);
        return error;
        
    }
    
    
    

    
};

#pragma mark
#include "OMX_Maps.h"

struct FilterParam
{
    string label="";
    int index=0;
    int min=0;
    int max=0;
    int defaultValue=0;
};

class FilterParamConfig
{
public:
    string name;
    vector<FilterParam> params;
    
    FilterParamConfig()
    {
        name="";
    }
    void addParam(string label, int min_, int max_, int defaultValue_)
    {
        FilterParam param;
        param.label = label;
        param.index = params.size();
        param.min = min_;
        param.max = max_;
        param.defaultValue = defaultValue_;
        params.push_back(param);
    }
    ofJson toJSON()
    {
        ofJson json;
        json["name"]=name;
  
        for(size_t i=0; i<params.size(); i++)
        {
            ofJson paramJSON;
            paramJSON["label"] = params[i].label;
            paramJSON["index"] = params[i].index;
            paramJSON["min"] = params[i].min;
            paramJSON["max"] = params[i].max;
            paramJSON["defaultValue"] = params[i].defaultValue;
            json["params"][i]=paramJSON; 
        }
        return json;
    }
};

class ImageFXController
{
public:
    
    
    OMX_HANDLETYPE component;
    int componentPort;
    vector<FilterParamConfig> filterParamConfigs;
    
    ImageFXController()
    {
        component = NULL;
        componentPort = OMX_ALL;
        filterParamConfigs.clear();
        
        for (auto& it : OMX_Maps::getInstance().imageFilters)
        {            
            FilterParamConfig filterParamConfig = createFilterParamConfig(it.second);
            if(!filterParamConfig.params.empty())
            {
                filterParamConfigs.push_back(filterParamConfig);
                
                ofLogNotice(__func__) << filterParamConfig.name << " HAS " << filterParamConfig.params.size() << " PARAMS";
            }
        }
        
        ofJson test  = getFilterParamConfigJson();
    }
    
    ofJson getFilterParamConfigJson()
    {
        ofJson json;
        for(size_t i=0; i<filterParamConfigs.size(); i++)
        {
            ofJson filterParamConfigJSON = filterParamConfigs[i].toJSON();
            json["filterParamConfigs"][i] = filterParamConfigJSON;   
        }
        
        ofLogNotice(__func__) << json.dump();
        
        return json;
    }
    
    void setup(OMX_HANDLETYPE component_, int componentPort_)
    {

        component = component_;
        componentPort = componentPort_;
        //setImageFilter(OMX_ImageFilterNone);
        
        
    }
    
    OMX_ERRORTYPE setImageFilter(string imageFilterName)
    {
        OMX_IMAGEFILTERTYPE imageFilter = GetImageFilter(imageFilterName);
        return setImageFilter(imageFilter);
    }
    
    
    OMX_ERRORTYPE setImageFilter(OMX_IMAGEFILTERTYPE imageFilter)
    {
        if(!component) return OMX_ErrorNotReady;
        
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
        if(!component) return OMX_ErrorNotReady;

        OMX_ERRORTYPE error  = setImageFilter(imageFilter);
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
        
        if(!component) return OMX_ErrorNotReady;
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
    
    
    FilterParamConfig createFilterParamConfig(OMX_IMAGEFILTERTYPE imageFilter)
    {
        FilterParamConfig filterParamConfig;
        filterParamConfig.name = GetImageFilterString(imageFilter);

        switch (imageFilter) 
        {
            case OMX_ImageFilterSolarize:
            {
                //Linear mapping of [0,x0] to [0,y0>] and [x0,255] to [y1,y2]. Default is "128 128 128 0".
                filterParamConfig.addParam("x1", 0, 255, 128);
                filterParamConfig.addParam("x2", 0, 255, 128);
                filterParamConfig.addParam("y1", 0, 255, 128);
                filterParamConfig.addParam("y2", 0, 255, 0);
                break;
            }
            case OMX_ImageFilterSharpen:
            {
                //sz size of filter, either 1 or 2. str strength of filter. th threshold of filter. Default is "1 40 20".
                filterParamConfig.addParam("size", 1, 2, 1);
                filterParamConfig.addParam("strength", 0, 255, 40);
                filterParamConfig.addParam("threshold", 0, 255, 20);                    
                break;
            }
            case OMX_ImageFilterFilm:
            {

                /*
                 str strength of effect. u sets u to constant value. v sets v to constant value. Default is "24".
                 */
                filterParamConfig.addParam("strength", 0, 255, 1);
                filterParamConfig.addParam("u", 0, 255, 24);
                filterParamConfig.addParam("v", 0, 255, 24);     
                break;
            }
            case OMX_ImageFilterBlur:
            {
                filterParamConfig.addParam("strength", 0, 2, 2);
                break;
            }
            case OMX_ImageFilterSaturation:
            {
                //str strength of effect, in 8.8 fixed point format. u/v value differences from 128 are multiplied by str. Default is "272".
                filterParamConfig.addParam("strength", 0, 1024, 272);
                break;
            }
            default:
            {
                break;
            }
        }
        return filterParamConfig;
    }    

    
};

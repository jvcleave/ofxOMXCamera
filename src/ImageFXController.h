#pragma mark
#include "OMX_Maps.h"

struct FilterParam
{
    int index;
    int min;
    int max;
    int defaultValue;
};

class FilterParamConfig
{
public:
    string name;
    vector<FilterParam> params;
    
    FilterParamConfig()
    {
        
    }
    void addParam(int min_, int max_, int defaultValue_)
    {
        FilterParam param;
        param.index = params.size();
        param.min = min_;
        param.max = max_;
        param.defaultValue = defaultValue_;
        params.push_back(param);
    }
};

class ImageFXController
{
public:
    
    
    OMX_HANDLETYPE imageFX;
    

    ImageFXController()
    {
        imageFX = NULL;
        for (auto& it : OMX_Maps::getInstance().imageFilters)
        {
            ofLogNotice(__func__) << it.first;
            
            FilterParamConfig filterParamConfig = createFilterParamConfig(it.second);
            if(!filterParamConfig.params.empty())
            {
                ofLogNotice(__func__) << filterParamConfig.name << " HAS " << filterParamConfig.params.size() << " PARAMS";
            }
        }
    }
    void setup(OMX_HANDLETYPE imageFX_)
    {

        imageFX = imageFX_;
        
        
        
    }
    
    OMX_ERRORTYPE setImageFilter(string imageFilterName)
    {
        OMX_IMAGEFILTERTYPE imageFilter = GetImageFilter(imageFilterName);
        return setImageFilter(imageFilter);
    }
    
    
    OMX_ERRORTYPE setImageFilter(OMX_IMAGEFILTERTYPE imageFilter)
    {
        if(!imageFX) return OMX_ErrorNotReady;
        
        OMX_CONFIG_IMAGEFILTERTYPE imagefilterConfig;
        OMX_INIT_STRUCTURE(imagefilterConfig);
        imagefilterConfig.nPortIndex = IMAGE_FX_OUTPUT_PORT;
        imagefilterConfig.eImageFilter = imageFilter;
        
        OMX_ERRORTYPE error = OMX_SetConfig(imageFX, OMX_IndexConfigCommonImageFilter, &imagefilterConfig);
        OMX_TRACE(error); 
        return error;
    }
    
    OMX_ERRORTYPE setImageFilter(OMX_IMAGEFILTERTYPE imageFilter, vector<int> params)
    {
        if(!imageFX) return OMX_ErrorNotReady;

        OMX_ERRORTYPE error  = setImageFilter(imageFilter);
        OMX_CONFIG_IMAGEFILTERPARAMSTYPE filtersParams;
        OMX_INIT_STRUCTURE(filtersParams);
        filtersParams.nPortIndex = IMAGE_FX_OUTPUT_PORT;
        filtersParams.eImageFilter = imageFilter;
        filtersParams.nNumParams = params.size();
        for(int i=0; i<params.size(); i++)
        {
            filtersParams.nParams[i] = params[i];        
        }
        error =  OMX_SetConfig(imageFX, OMX_IndexConfigCommonImageFilterParameters, &filtersParams);
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
                filterParamConfig.addParam(0, 255, 128);
                filterParamConfig.addParam(0, 255, 128);
                filterParamConfig.addParam(0, 255, 128);
                filterParamConfig.addParam(0, 255, 0);
                break;
            }
            case OMX_ImageFilterSharpen:
            {
                //sz size of filter, either 1 or 2. str strength of filter. th threshold of filter. Default is "1 40 20".
                filterParamConfig.addParam(1, 2, 1);
                filterParamConfig.addParam(0, 255, 40);
                filterParamConfig.addParam(0, 255, 20);                    
                break;
            }
            case OMX_ImageFilterFilm:
            {

                /*
                 str strength of effect. u sets u to constant value. v sets v to constant value. Default is "24".
                 */
                filterParamConfig.addParam(0, 255, 1);
                filterParamConfig.addParam(0, 255, 24);
                filterParamConfig.addParam(0, 255, 24);     
                break;
            }
            case OMX_ImageFilterBlur:
            {
                filterParamConfig.addParam(0, 2, 2);
                break;
            }
            case OMX_ImageFilterSaturation:
            {
                //str strength of effect, in 8.8 fixed point format. u/v value differences from 128 are multiplied by str. Default is "272".
                filterParamConfig.addParam(0, 1024, 272);
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

#pragma once

#include "ofMain.h"
#include "OMX_Maps.h"
#include "ofAppEGLWindow.h"



class DisplayController
{
public:
    
    
    EGLImageKHR eglImage;
    EGLDisplay display;
    EGLContext context;
    unsigned char* pixels;
    ofTexture texture;
    ofAppEGLWindow* appEGLWindow;
    unsigned int textureID;
    ofFbo fbo;
    
    
    OMX_CONFIG_DISPLAYREGIONTYPE displayConfig;
    OMX_CONFIG_DISPLAYREGIONTYPE displayConfigDefaults;
    OMX_HANDLETYPE renderComponent;
    bool doHDMISync;    

    bool doFullScreen;
    bool noAspectRatio;
    bool doMirror;
    int rotationIndex;
    int rotationDegrees;
    int alpha;
    bool doForceFill;
    int layer;
    bool textureMode;
    bool isTextureEnabled()
    {
        return textureMode;
    }
    
    ofxOMXCameraSettings* settings;
    ofPixels of_pixels;
    DisplayController()
    {
        doFullScreen=false;
        noAspectRatio=false;
        doMirror=false;
        rotationIndex=0;
        
        doHDMISync = true;
        alpha = 255;
        layer = 0;
        doMirror = false;
        rotationIndex = 0;
        rotationDegrees =0; 
        doForceFill = false;
        
        OMX_INIT_STRUCTURE(displayConfig);
        displayConfig.nPortIndex = VIDEO_RENDER_INPUT_PORT;
        displayConfigDefaults = displayConfig;
        
        
        eglImage = NULL;
        display = NULL;
        context = NULL;
        appEGLWindow = NULL;
        pixels = NULL;
        textureID = 0;
        textureMode = false;
        settings = NULL;
        
    };
    
    
    ~DisplayController()
    {
        close();
    }
    
    void close()
    {
        renderComponent = NULL;
        destroyEGLImage();
        eglImage = NULL;
        display = NULL;
        context = NULL;
        appEGLWindow = NULL;
        settings = NULL;
        if(pixels)
        {
            delete[] pixels;
            pixels = NULL;
        }
    }

    
    void setup(ofxOMXCameraSettings* settings_, OMX_HANDLETYPE renderComponent_=NULL)
    {
        settings = settings_;
        if(settings->enableTexture)
        {
            textureMode = true;
        }else
        {
            textureMode = false;
            destroyEGLImage();
            renderComponent = renderComponent_;
            applyConfig();
        }
    }
    
    
    void updateTexture(bool pixelsRequested=false)
    {
        fbo.begin();
        ofClear(0, 0, 0, 0);
        texture.draw(0, 0);
        if (pixelsRequested) 
        {
            glReadPixels(0, 0,
                         texture.getWidth(),
                         texture.getHeight(),
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         pixels);    
        }
        fbo.end();
    }
    void draw()
    {
        draw(0, 0);
    }
    
    
    void draw(int x, int y)
    {
        
        if(!settings)return;
        ofRectangle rect(x, y, settings->drawRectangle.width, settings->drawRectangle.height);
        draw(rect);
    }
    
    
    void draw(int x, int y, int width, int height)
    {
        
        draw(ofRectangle(x, y, width, height));
    }
    
    void draw(const ofRectangle& rectangle)
    {
        
        if(!settings)
        {
            ofLogError(__func__ ) << "NO SETTINGS";
            return;
        }
        bool didChange = false;
        if(settings->drawRectangle.x != rectangle.x ||
           settings->drawRectangle.y != rectangle.y ||
           settings->drawRectangle.width != rectangle.width ||
           settings->drawRectangle.height != rectangle.height )
        {
            settings->drawRectangle.set(rectangle.x, rectangle.y, rectangle.width, rectangle.height);
            didChange = true;
            //ofLogNotice(__func__) << "DID CHANGE drawRectangle: " << settings->drawRectangle;
        }else
        {
            //ofLogNotice(__func__) << "drawRectangle: " << drawRectangle;
        }
        //

        if(isTextureEnabled())
        {
            fbo.draw(settings->drawRectangle.x, settings->drawRectangle.y, settings->drawRectangle.width, settings->drawRectangle.height);
        }else
        {
            if(didChange)
            {
                applyConfig();
            }
        }
    }
    void applyConfig()
    {
        if(isTextureEnabled()) return;
        if(!renderComponent) return;
        if(!settings)return;
        
        displayConfig.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_DEST_RECT /*| OMX_DISPLAY_SET_SRC_RECT */ | OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_NOASPECT | OMX_DISPLAY_SET_TRANSFORM | OMX_DISPLAY_SET_ALPHA | OMX_DISPLAY_SET_LAYER | OMX_DISPLAY_SET_MODE);
        
        displayConfig.dest_rect.x_offset  = settings->drawRectangle.x;
        displayConfig.dest_rect.y_offset  = settings->drawRectangle.y;
        displayConfig.dest_rect.width     = settings->drawRectangle.width;
        displayConfig.dest_rect.height    = settings->drawRectangle.height;
        //ofLog() << "drawRectangle: " << drawRectangle;
        /*displayConfig.src_rect.x_offset  = settings->cropRectangle.x;
        displayConfig.src_rect.y_offset  = settings->cropRectangle.y;
        displayConfig.src_rect.width     = settings->cropRectangle.width;
        displayConfig.src_rect.height    = settings->cropRectangle.height;*/
        
        displayConfig.fullscreen = (OMX_BOOL)doFullScreen;
        displayConfig.noaspect   = (OMX_BOOL)noAspectRatio;    
        displayConfig.transform  = (OMX_DISPLAYTRANSFORMTYPE)rotationIndex;
        //int alpha = (ofGetFrameNum() % 255); 
        displayConfig.alpha  = alpha;
        displayConfig.layer  = layer;

        if(doForceFill)
        {
            displayConfig.mode  = OMX_DISPLAY_MODE_FILL;  
        }else
        {
            displayConfig.mode = displayConfigDefaults.mode;
        }
        //displayConfig.mode  = OMX_DISPLAY_MODE_FILL;
        //displayConfig.mode  = (OMX_DISPLAYMODETYPE)ofRandom(0, 5);
        //return OMX_ErrorNone;
        
        OMX_ERRORTYPE error  = OMX_SetParameter(renderComponent, OMX_IndexConfigDisplayRegion, &displayConfig);
        OMX_TRACE(error);
    }

 
    
    
    void rotateDisplay(OMX_DISPLAYTRANSFORMTYPE type)
    {

        rotationIndex = (int)type;
        applyConfig();
    }
    
    
    void rotateDisplay(int degreesClockWise)
    {
        OMX_DISPLAYTRANSFORMTYPE type = OMX_DISPLAY_ROT0;
        
        if(degreesClockWise<0)
        {
            type = OMX_DISPLAY_ROT0;
        }
        if(degreesClockWise >=90 && degreesClockWise < 180)
        {
            type = OMX_DISPLAY_ROT90;
        }
        if(degreesClockWise >=180 && degreesClockWise < 270)
        {
            type = OMX_DISPLAY_ROT270;
        }
        
        if(doMirror)
        {
            switch (type) 
            {
                case OMX_DISPLAY_ROT0:
                {
                    type = OMX_DISPLAY_MIRROR_ROT0;
                    break;
                }
                case OMX_DISPLAY_ROT90:
                {
                    type = OMX_DISPLAY_MIRROR_ROT90;
                    break;
                }
                case OMX_DISPLAY_ROT270:
                {
                    type = OMX_DISPLAY_MIRROR_ROT270;
                    break;
                }
                    
                default:
                    break;
            }
        }
        rotateDisplay(type);
    }
    

    
    void setDisplayAlpha(int alpha_)
    {
        alpha = alpha_;
        applyConfig();
    }
    
    void setDisplayLayer(int layer_)
    {
        layer = layer_;
        applyConfig();
    }
    
    void setDisplayRotation(int rotationDegrees_)
    {
        rotateDisplay(rotationDegrees_);
        applyConfig();
    }
    
    void setDisplayDrawRectangle(ofRectangle rect)
    {
        if(!settings) return;
        
        settings->drawRectangle.set(rect.x, rect.y, rect.width, rect.height);
        applyConfig();
    }
    
    void setDisplayCropRectangle(ofRectangle cropRectangle_)
    {
        if(!settings) return;
        settings->cropRectangle = cropRectangle_;
        OMX_CONFIG_DISPLAYREGIONTYPE cropConfig;
        OMX_INIT_STRUCTURE(cropConfig);
        cropConfig.nPortIndex = VIDEO_RENDER_INPUT_PORT;        
        cropConfig.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_SRC_RECT);
        cropConfig.src_rect.x_offset  = settings->cropRectangle.x;
        cropConfig.src_rect.y_offset  = settings->cropRectangle.y;
        cropConfig.src_rect.width     = settings->cropRectangle.width;
        cropConfig.src_rect.height    = settings->cropRectangle.height;
        OMX_ERRORTYPE error  = OMX_SetParameter(renderComponent, OMX_IndexConfigDisplayRegion, &cropConfig);
        OMX_TRACE(error);
    }
    
    void setDisplayMirror(bool doMirror_)
    {
        doMirror = doMirror_;
        applyConfig();
    }
    

    string toString()
    {
        stringstream info;
        info << "isTextureEnabled: " << isTextureEnabled() << endl;
        info << "drawRectangle: " << settings->drawRectangle << endl;

        if(isTextureEnabled())
        {
            info << "fullscreen: " << displayConfig.fullscreen << endl; 
            info << "noaspect: " << displayConfig.noaspect << endl;
            info << "src_rect x: " << displayConfig.src_rect.x_offset << endl;  
            info << "src_rect y: " << displayConfig.src_rect.y_offset << endl;  
            info << "src_rect width: " << displayConfig.src_rect.width << endl;    
            info << "src_rect height: " << displayConfig.src_rect.height << endl;    
            
            info << "dest_rect x: " << displayConfig.dest_rect.x_offset << endl; 
            info << "dest_rect y: " << displayConfig.dest_rect.y_offset << endl; 
            info << "dest_rect width: " << displayConfig.dest_rect.width << endl;    
            info << "dest_rect height: " << displayConfig.dest_rect.height << endl;
            
            info << "transform: " << displayConfig.transform << endl;
            
            
            info << "transform: " << displayConfig.transform << endl;
            
            info << "mode: " << displayConfig.mode << endl;
            
            info << "layer: " << displayConfig.layer << endl;
            info << "alpha: " << displayConfig.alpha << endl;
        }


        return info.str();
    }
    
    bool generateEGLImage(int videoWidth, int videoHeight)
    {
        bool success = false;
        bool needsRegeneration = false;
        
        if (!texture.isAllocated())
        {
            needsRegeneration = true;
        }
        else
        {
            if (texture.getWidth() != videoWidth && texture.getHeight() != videoHeight)
            {
                needsRegeneration = true;
            }
        }
        
        if (!fbo.isAllocated())
        {
            needsRegeneration = true;
        }
        else
        {
            if (fbo.getWidth() != videoWidth && fbo.getHeight() != videoHeight)
            {
                needsRegeneration = true;
            }
        }
        
        if(!needsRegeneration)
        {
            //ofLogVerbose(__func__) << "NO CHANGES NEEDED - RETURNING EARLY";
            return true;
        }
        
        if (appEGLWindow == NULL)
        {
            appEGLWindow = (ofAppEGLWindow *) ofGetWindowPtr();
        }
        
        if (appEGLWindow == NULL)
        {
            ofLogError(__func__) << "appEGLWindow is NULL - RETURNING";
            return false;
        }
        if (display == NULL)
        {
            display = appEGLWindow->getEglDisplay();
        }
        if (context == NULL)
        {
            context = appEGLWindow->getEglContext();
        }
        
        if (display == NULL)
        {
            ofLogError(__func__) << "display is NULL - RETURNING";
            return false;
        }
        if (context == NULL)
        {
            ofLogError(__func__) << "context is NULL - RETURNING";
            return false;
        }
        
        if (needsRegeneration)
        {
            
            fbo.allocate(videoWidth, videoHeight, GL_RGBA);
            texture.allocate(videoWidth, videoHeight, GL_RGBA);
            texture.setTextureWrap(GL_REPEAT, GL_REPEAT);
            textureID = texture.getTextureData().textureID;
        }
        
        ofLog() << "textureID: " << textureID;
        ofLog() << "tex.isAllocated(): " << texture.isAllocated();
        ofLog() << "videoWidth: " << videoWidth;
        ofLog() << "videoHeight: " << videoHeight;
        ofLog() << "pixels: " << videoHeight;
        
        // setup first texture
        int dataSize = videoWidth * videoHeight * 4;
        
        if (pixels && needsRegeneration)
        {
            delete[] pixels;
            pixels = NULL;
        }
        
        if (pixels == NULL)
        {
            pixels = new unsigned char[dataSize];
        }
        ofLog() << "dataSize: " << dataSize;
        
        //memset(pixels, 0xff, dataSize);  // white texture, opaque
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, videoWidth, videoHeight, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        
        
        if (eglImage && needsRegeneration)
        {
            destroyEGLImage();
        }
        
        // Create EGL Image
        eglImage = eglCreateImageKHR(display, context, EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)textureID, NULL);
        
        if (eglImage == EGL_NO_IMAGE_KHR)
        {
            ofLog()    << "Create EGLImage FAIL <---------------- :(";
            
        }
        else
        {
            success = true;
            ofLog()  << "Create EGLImage PASS <---------------- :)";
            of_pixels.setFromExternalPixels(pixels, videoWidth, videoHeight, 4);
        }
        return success;
        
    }
    void destroyEGLImage()
    {
        
        
        if (eglImage)
        {
            if (appEGLWindow == NULL)
            {
                appEGLWindow = (ofAppEGLWindow *) ofGetWindowPtr();
            }
            
            if(!appEGLWindow) return;
            
            if (display == NULL)
            {
                display = appEGLWindow->getEglDisplay();
            }
            
            if(!display) return;
            
            if (!eglDestroyImageKHR(display, eglImage))
            {
                ofLog() << __func__ << " FAIL <---------------- :(";
            }else
            {
                ofLog() << __func__ << " PASS <---------------- :)";
            }
            eglImage = NULL;
        }
        
    }
    
 
               
};

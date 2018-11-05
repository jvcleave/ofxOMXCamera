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
    ofRectangle drawRectangle;
    ofRectangle cropRectangle;
    bool doFullScreen;
    bool noAspectRatio;
    bool doMirror;
    int rotationIndex;
    int rotationDegrees;
    int alpha;
    bool doForceFill;
    int layer;
    
    bool isTextureEnabled()
    {
        return eglImage != NULL;
    }
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
        if(pixels)
        {
            delete[] pixels;
            pixels = NULL;
        }
    }
    void setupTextureMode(int x, int y, int width, int height)
    {
        drawRectangle.set(x, y, width, height);
    }
    
    void setupDirectMode(OMX_HANDLETYPE renderComponent_, int x, int y, int width, int height)
    {
        renderComponent = renderComponent_;
        drawRectangle.set(x, y, width, height);
        cropRectangle.set(x, y, width, height);
        TRACE_LINE
        applyConfig();
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
        draw(drawRectangle);
    }
    
    
    void draw(int x, int y)
    {
        draw(x, y, drawRectangle.width, drawRectangle.height);
    }
    
    
    void draw(int x, int y, int width, int height)
    {
        draw(ofRectangle(x, y, width, height));
    }
    void draw(ofRectangle rectangle)
    {
        bool didChange = false;
        if(drawRectangle.x != rectangle.x &&
           drawRectangle.y != rectangle.y &&
           drawRectangle.width != rectangle.width &&
           drawRectangle.height != rectangle.height )
        {
            drawRectangle = rectangle;
            didChange = true;
        }
        if(isTextureEnabled())
        {
            fbo.draw(drawRectangle.x, drawRectangle.y, drawRectangle.width, drawRectangle.height);
            
        }else
        {
            setDisplayDrawRectangle(drawRectangle);
        }
    }
    void applyConfig()
    {
        if(!renderComponent) return;
        
        displayConfig.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_DEST_RECT| OMX_DISPLAY_SET_SRC_RECT | OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_NOASPECT | OMX_DISPLAY_SET_TRANSFORM | OMX_DISPLAY_SET_ALPHA | OMX_DISPLAY_SET_LAYER | OMX_DISPLAY_SET_MODE);
        
        displayConfig.dest_rect.x_offset  = drawRectangle.x;
        displayConfig.dest_rect.y_offset  = drawRectangle.y;
        displayConfig.dest_rect.width     = drawRectangle.getWidth();
        displayConfig.dest_rect.height    = drawRectangle.getHeight();
        //ofLog() << "drawRectangle: " << drawRectangle;
        displayConfig.src_rect.x_offset  = cropRectangle.x;
        displayConfig.src_rect.y_offset  = cropRectangle.y;
        displayConfig.src_rect.width     = cropRectangle.getWidth();
        displayConfig.src_rect.height    = cropRectangle.getHeight();
        
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
    
    void setDisplayDrawRectangle(ofRectangle drawRectangle_)
    {
        drawRectangle = drawRectangle_;
        applyConfig();
    }
    
    void setDisplayCropRectangle(ofRectangle cropRectangle_)
    {
        cropRectangle = cropRectangle_;
        applyConfig();
    }
    
    void setDisplayMirror(bool doMirror_)
    {
        doMirror = doMirror_;
        applyConfig();
    }
    

    string toString()
    {
        stringstream info;
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
        
        
        
        
        info << "drawRectangle: " << drawRectangle << endl;
        info << "drawRectangle.getArea(): " << drawRectangle.getArea() << endl;
        
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

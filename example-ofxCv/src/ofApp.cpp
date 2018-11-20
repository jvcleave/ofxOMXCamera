#include "ofApp.h"



//--------------------------------------------------------------
void ofApp::setup()
{
    
    doEraseBackground = true;
   
    alpha = 0.01;
    threshold = 18;
    
    
	ofSetLogLevel(OF_LOG_VERBOSE);			
	consoleListener.setup(this);
    
    settings.sensorWidth = 320;
    settings.sensorHeight = 240;
    settings.framerate = 15;
    settings.enableTexture = true;
    settings.enablePixels = true;
    //settings.brightness = 50;
    
    videoGrabber.setup(settings);
    
  
}	

//--------------------------------------------------------------
void ofApp::update()
{
    
}


//--------------------------------------------------------------
void ofApp::draw(){
	
    
    if(videoGrabber.isFrameNew())
    {
        if(videoGrabber.getPixels().size())
        {
            
            
            if(frameMat.empty())
            {
                frameMat = ofxCv::toCv(videoGrabber.getPixels());

                frameMat.convertTo(accumulatorMat, CV_32F);
            }else
            {
                if(doEraseBackground)
                {
                    cv::accumulateWeighted(frameMat, accumulatorMat, alpha);
                    cv::convertScaleAbs(accumulatorMat, backgroundOutputMat); 
                    
                }
            }            
            
        } 
        
    }
    
    if(!backgroundOutputMat.empty())
    {
        ofxCv::toOf(backgroundOutputMat, backgroundOutputImage);
        backgroundOutputImage.update();
        backgroundOutputImage.draw(0, 0,
                                   ofGetWidth(), ofGetHeight());
    }
    
    float xScale = settings.sensorWidth;
    float yScale = settings.sensorHeight;
    videoGrabber.draw(ofGetWidth()-xScale, ofGetHeight()-yScale, xScale, yScale);
    
	stringstream info;
	info << "APP FPS: " << ofGetFrameRate() << endl;
	info << "Camera Resolution: " << videoGrabber.getWidth() << "x" << videoGrabber.getHeight()	<< " @ "<< videoGrabber.getFrameRate() <<"FPS"<< endl;
    info << "doEraseBackground: " << doEraseBackground << endl;
    info << "threshold: " << threshold << endl;
    info << "alpha: " << alpha << endl;

	info << endl;


    ofDrawBitmapStringHighlight(info.str(), 100, 100, ofColor(ofColor::black, 90), ofColor::yellow);
    

}

//--------------------------------------------------------------
void ofApp::keyPressed  (int key)
{
	ofLog(OF_LOG_VERBOSE, "%c keyPressed", key);
    switch (key) 
    {
        case '1':
        {
            doEraseBackground = !doEraseBackground;
            break;
        }
        case '2':
        {
            break;
        }  
        case '3':
        {
            threshold--;
            break;
        } 

        case '4':
        {
            threshold++;
            break;
        }

        case '5':
        {
            alpha += 0.01;
            break;
        }
        case '6':
        {
            alpha -= 0.01;
            break;
        } 
 
        default:
        {
            break;
        }
        
    }
}


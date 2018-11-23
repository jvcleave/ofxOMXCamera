#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
	
	//allows keys to be entered via terminal remotely (ssh)
	consoleListener.setup(this);
	
	
	settings.sensorWidth = 1280; //default 1280
	settings.sensorHeight = 720; //default 720
	settings.enableTexture = true; //default true
    settings.enableExtraFilters = true;

	//pass in the settings and it will start the camera
	videoGrabber.setup(settings);
	
	//ImageFilterCollection (filterCollection here) is helper class to iterate through available OpenMax filters
	filterCollection.setup();


}

//--------------------------------------------------------------
void ofApp::update()
{
	

}


//--------------------------------------------------------------
void ofApp::draw(){

	//draws at camera resolution
	videoGrabber.draw();
	
    if(videoGrabber.isTextureEnabled())
    {
        //draw a smaller version via the getTextureReference() method
        int drawWidth = settings.sensorWidth/4;
        int drawHeight = settings.sensorHeight/4;
        videoGrabber.getTextureReference().draw(settings.sensorWidth-drawWidth, settings.sensorHeight-drawHeight, drawWidth, drawHeight);
    }


	stringstream info;
	info << "App FPS: " << ofGetFrameRate() << endl;
	info << "Camera Resolution: " << videoGrabber.getWidth() << "x" << videoGrabber.getHeight()	<< " @ "<< videoGrabber.getFrameRate() <<"FPS"<< endl;
	info << "CURRENT FILTER: " << filterCollection.getCurrentFilterName() << endl;	
	//info <<	filterCollection.filterList << "\n";
	
    info << endl;
	info << "Press 1 to increment filter" << endl;

	
    ofDrawBitmapStringHighlight(info.str(), 100, 100, ofColor::black, ofColor::yellow);

}

//--------------------------------------------------------------
void ofApp::keyPressed  (int key)
{
	ofLog(OF_LOG_VERBOSE, "%c keyPressed", key);
	
	
	if (key == '1')
	{
		videoGrabber.setImageFilter(filterCollection.getNextFilter());
	}
    if (key == '2')
    {        
        videoGrabber.setExtraImageFilter(GetImageFilterString(filterCollection.getNextFilter()));
    }
}

void ofApp::onCharacterReceived(KeyListenerEventData& e)
{
	keyPressed((int)e.character);
}


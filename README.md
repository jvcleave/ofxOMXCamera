#### STATUS   
Master may be unstable, features untested. See [Releases](https://github.com/jvcleave/ofxOMXCamera/releases) for tested versions





#### DESCRIPTION:   
openFrameworks addon to control the Raspberry Pi Camera Module. Formerly [ofxRPiCameraVideoGrabber](https://github.com/jvcleave/ofxRPiCameraVideoGrabber) that only provided video functionality. This addon also provides still capture functionality as well as the abilty to save settings as JSON based files.


#### REQUIREMENTS:
openFrameworks 0.10.*, Raspberry Pi 1-3    [Setup Guide](http://openframeworks.cc/setup/raspberrypi/)   
Developed with GPU memory set at 256, overclock to medium but 128/default should work as well   
Desktop Mode (X11 enabled) may work but untested

LED Toggling requires gpio program provided via wiringPi   
`$sudo apt-get install wiringpi`

#### USAGE:   
Clone into your openFrameworks/addons folder

#### Primary classes
##### ofxOMXVideoGrabber:
Used for video capture and recording. Video is hardware accelerated and written to a texture (texture mode) or directly to the screen. Texture Mode allows:
 - Shaders
 - Pixel access
 - Overlays, etc

##### ofxOMXPhotoGrabber
Used for controlling still functionality of the camera. Only hardware accelerated features are enabled (e.g. JPG compression). Similar to video mode, a preview can be rendered to a texture or directly to the screen.


##### RECORDING:   
Recording is available in both texture and non-texture modes


### EXAMPLES:   

#### VIDEO
##### example-demo-mode    
Shows different settings available to tweak the camera exposure, metering, cropping, zooming, filters, mirroring, white balance

##### example-direct-mode   
Camera turns on and is rendered full screen via OMX acceleration   
Press the "e" key to toggle through built in filters

##### example-direct-mode-transform
Demos cropping, alpha, mirroring of direct display (not camera)  

##### example-texture-mode  
Camera turns on and renders to a texture that is drawn at full screen and a scaled version   
Press the "e" key to toggle through built in filters   


##### example-shaders   
Basic shader usage with texture-mode  
Press the "e" key to toggle through built in filters 
Press the "s" key to toggle shader   

##### example-saved-settings:   
Alternative way to load a camera configuration through a text file

##### example-recording:   
Recording of video in texture or direct mode

##### example-wrapper:   
Drop-in replacement for ofVideoGrabber (texture-mode only)

#### STILL
##### example-still:   
Still camera functionality






 





 

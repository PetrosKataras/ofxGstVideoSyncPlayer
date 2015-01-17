# ofxGstNetVideoPlayer

An openFrameworks addon for playing back videos in sync over the network with GStreamer.

This addon utilizes the [gstreamer network clocks](http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer-libs/html/gstreamer-net.html) for syncing video playback over the network on two or more computers.

It also uses ofxOsc for master/client communication ( play, pause etc. ) 

It has been tested and developed with master OF 0.9.0 from github(cab205321) under Xubuntu 14.04 and OS X 10.8.5 
with gstreamer 1.4.4 and gstreamer 1.4.5 ( although in theory it should work with any gstreamer version above 1.0 ) .

This addon should be considered WIP. Basic functionality for play - pause and looping in a synchronized manner is there but
there are rough edges ( especially related to resuming after pausing ) that need further work and testing + additions for seeking and so on...

## Installation - Dependencies

For Linux you should already have all dependencies installed as part of the OF core.

For OS X & Windows you will need [ofxGStreamer](https://github.com/arturoc/ofxGStreamer.git) from Arturo Castro.
Follow the installation instructions that you find there in order to properly install GStreamer for your platform.

## Compiling the example

The [addon_config.mk](https://github.com/PetrosKataras/ofxGstVideoSyncPlayer/blob/master/addon_config.mk) file should pull necessary dependencies based on the platform that you are working..

If you prefer the manual way here is how to:

`ofxOsc` is a dependency for all platforms and you will need to add it in your projects `addons.make` file.

For Linux you will have to explicitely link to `libgstnet-1.0`. You can do this by setting:

`PROJECT_LDFLAGS=-lgstnet-1.0` in your projects `config.make` file.

For OS X you will need to add `ofxGStreamer` in your projects `addons.make` file ( in addition to ofxOsc ).

The example project configuration assumes a command line configuration and development. For CB / XCode / VS project templates we would have to look to the community or the project generator.

NOTE: With current master of ofxGStreamer (114bbc84b2) and OF (cab205321) under OS X I had to remove ofxGStreamer.h and ofxGStreamer.cpp from the [addon_config.mk](https://github.com/arturoc/ofxGStreamer/blob/master/addon_config.mk#L118) in order to successfully compile a project that uses ofxGStreamer. There is a weirdness with paths happening but didnt investigate more.  

## Usage & Concept

One instance acts as the master instance and subsequent instances can be started on different machines as clients that will keep in sync with this master instance. The master should always be started first before any clients. Only one client instance is supported per machine. The machine running the server can also run a slave instance. 

The clients will poll the clock of the pipeline running on the master machine and calibrate their time in order to keep in sync. This is all handled internally from GStreamer.

For sample usage check the included example.

## Licence

The code found in this repository is distributed under the [MIT Licence](http://opensource.org/licenses/MIT).

Copyright (c) 2015 Petros Kataras

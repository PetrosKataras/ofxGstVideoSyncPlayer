#pragma once

#include "ofMain.h"
#include "ofxGstVideoSyncPlayer.h"
#include "segment.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
	
        ofxGstVideoSyncPlayer player;
        Segment playerSegment1;
        Segment playerSegment2;
        Segment playerSegment3;
        Segment playerSegment4;

};

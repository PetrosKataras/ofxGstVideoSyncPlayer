#include "ofApp.h"
#include "ofGstVideoPlayer.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetLogLevel(OF_LOG_VERBOSE);
    ///> Pass here the IP and the port the master is running.
    ///> False if this is a client, true if we are on the master.
    ///> Internally the next two ports from the one you specify here
    ///> are going to be used for the osc communication.
    ///> So in this case 1234 will be used for the gstreamer synchronization
    ///> and 1235 - 1236 for the osc communication between the master and the client.
    player.init("127.0.0.1", 1234, false);
    player.load("fingers.mov");
    player.loop(true);
    player.play();
}

//--------------------------------------------------------------
void ofApp::update(){

    player.update();
}

//--------------------------------------------------------------
void ofApp::draw(){

    player.draw(ofPoint(0,0));

    ///>master
    //player.drawSubsection(0.0f, 0.0f, player.getWidth()/2.0f, player.getHeight(), 0.0f, 0.0f);
    ///>client
    //player.drawSubsection(0.0f, 0.0f, player.getWidth()/2.0f, player.getHeight(), player.getWidth()/2.0f, 0.0f);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

    if( key == 'p' ){
        player.pause();
    }
    else if( key == 's' ){
        player.play();
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

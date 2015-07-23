#pragma once

#include "ofMain.h"

class Segment{
    public:
        Segment(){}

        void setup( const int _width, const int _height );
        void setOrigin( int _x, int _y );
        void setPos( int _x, int _y );
        void draw( ofTexture _tex );
    private:
        ofFbo m_segment;
        ofPoint m_origin;
        ofPoint m_pos;
};

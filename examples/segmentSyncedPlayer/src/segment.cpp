#include "segment.h"

void Segment::setup( int _width, int _height )
{
    m_segment.allocate( _width, _height );
    m_segment.begin();
    ofClear(255);
    m_segment.end();
}

void Segment::setOrigin( int _x, int _y )
{
    m_origin = ofPoint( _x, _y ); 
}

void Segment::setPos( int _x, int _y )
{
    m_pos = ofPoint( _x, _y );
}

void Segment::draw( ofTexture _tex )
{
    m_segment.begin();
    ofClear(255);
    ofPushMatrix();
    ofTranslate( -m_origin.x, -m_origin.y );
    _tex.draw(0, 0);
    ofPopMatrix();
    m_segment.end();

    m_segment.draw(m_pos.x, m_pos.y);

    
    
}

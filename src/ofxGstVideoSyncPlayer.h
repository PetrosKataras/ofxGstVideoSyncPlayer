#pragma once

#include "ofMain.h"
#include "ofGstVideoPlayer.h"
#include <gst/net/gstnet.h>
#include "ofxOsc.h"

typedef std::pair<std::string, int> ClientKey;
typedef std::set<ClientKey> Clients;
typedef Clients::iterator clients_iter;

class ofxGstVideoSyncPlayer{

    public:

        ofxGstVideoSyncPlayer();
        ~ofxGstVideoSyncPlayer();

        void                            initAsMaster( const std::string _clockIp, const int _clockPort, const int _oscRcvPort );
        void                            initAsSlave( const std::string _clockIp, const int _clockPort, const int _oscRcvPort, const int _oscSndPort );
        void                            loadAsync( std::string _path );
        bool                            load( std::string _path );
        void                            play();
        void                            update();
        void                            seek(gint64 position);
        gint64                          getPosition();
        unsigned long int               getPositionMilliseconds();
        gint64                          getDuration();
        unsigned long int               getDurationMilliseconds();
        void                            draw( ofPoint _pos, float _width = -1, float _height = -1 );
        void                            drawSubsection( float _x, float _y, float _w, float _h, float _sx, float _sy );
        void                            loop( bool _loop );
        void                            setVolume( float _volume );
        float                           getWidth();
        float                           getHeight();
        void                            pause();
        ofTexture                       getTexture();
        bool                            isPaused();
        bool                            isMovieEnded();
        bool                            isMaster();
        void                            exit(ofEventArgs & args);
        void                            setPixelFormat( const ofPixelFormat & _pixelFormat );
        const Clients&                  getConnectedClients();
    protected:

        Clients                         m_connectedClients; ///> Our connected clients.
        shared_ptr<ofxOscSender>        m_oscSender;        ///> osc sender.
        shared_ptr<ofxOscReceiver>      m_oscReceiver;      ///> osc receiver.

        bool                            m_isMaster;         ///> Is the master?
        bool                            m_initialized;      ///> If the player initialized properly ??
    private:

        void                            setMasterClock();
        void                            setClientClock( GstClockTime _baseTime );

        void                            sendPauseMsg();
        void                            sendPlayMsg();
        void                            sendLoopMsg();
        void                            sendEosMsg();

        void                            movieEnded( ofEventArgs & e );

    private:

        GstClock*                       m_gstClock;         ///> The network clock.
        GstElement*                     m_gstPipeline;      ///> The running pipeline.
        GstClockTime                    m_gstClockTime;     ///> The base time.
        shared_ptr<ofGstVideoPlayer>    m_gstPlayer;        ///> The gstreamer player.
        ofVideoPlayer                   m_videoPlayer;      ///> Our OF video player.
        std::string                     m_clockIp;          ///> The IP of the server.
        int                             m_clockPort;        ///> The port that should be used for the synchronization.
        int                             m_rcvPort;          ///> osc communication.
        int                             m_sndPort;          ///> osc communication.
        bool                            m_loop;             ///> Should we loop?
        bool                            m_movieEnded;       ///> Has the video ended??
        gint64                          m_pos;              ///> Position of the player.
        bool                            m_paused;           ///> Is the player paused ??
};

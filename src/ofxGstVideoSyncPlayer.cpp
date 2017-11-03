#include "ofxGstVideoSyncPlayer.h"

ofxGstVideoSyncPlayer::ofxGstVideoSyncPlayer()
    : m_gstClock(NULL)
    , m_gstPipeline(NULL)
    , m_gstClockTime(0)
    , m_isMaster(false)
    , m_clockIp("127.0.0.1")
    , m_clockPort(1234)
    , m_rcvPort(m_clockPort+1)
    , m_sndPort(m_clockPort+2)
    , m_loop(false)
    , m_initialized(false)
    , m_movieEnded(false)
    , m_paused(true)
{
    ofSetLogLevel("ofxGstVideoSyncPlayer", OF_LOG_VERBOSE);

    m_gstPlayer = shared_ptr<ofGstVideoPlayer>(new ofGstVideoPlayer);
    m_videoPlayer.setPlayer(m_gstPlayer);

    ofAddListener(ofEvents().exit, this, &ofxGstVideoSyncPlayer::exit);
}

ofxGstVideoSyncPlayer::~ofxGstVideoSyncPlayer()
{
    ofRemoveListener(ofEvents().exit, this, &ofxGstVideoSyncPlayer::exit);

    if( m_isMaster && m_initialized ){
        ofRemoveListener(m_gstPlayer->getGstVideoUtils()->eosEvent, this, &ofxGstVideoSyncPlayer::movieEnded);
        m_initialized = false;
    }
    else if( !m_isMaster && m_initialized ){
        ofxOscMessage m;
         m.setAddress("/client-exited");
         m.addInt64Arg(m_rcvPort);
         if( m_oscSender ){
             m_oscSender->sendMessage(m,false);
         }
         m_initialized = false;
    }
}

void ofxGstVideoSyncPlayer::exit(ofEventArgs & args)
{
    //> Triggered when app exits.
    if( !m_isMaster && m_initialized ){
        ofxOscMessage m;
        m.setAddress("/client-exited");
        m.addInt64Arg(m_rcvPort);
        if( m_oscSender ){
            m_oscSender->sendMessage(m,false);
        }
        m_initialized = false;
    }
}

void ofxGstVideoSyncPlayer::movieEnded( ofEventArgs & e )
{
    if( !m_movieEnded ){
        ofLogVerbose("ofxGstVideoSyncPlayer") << " Movie ended.. " << std::endl;
        m_movieEnded = true;
        m_paused = true;
        sendEosMsg();
    }
}

void ofxGstVideoSyncPlayer::loop( bool _loop )
{
    m_loop = _loop;
}

void ofxGstVideoSyncPlayer::setVolume( float _volume )
{
    m_videoPlayer.setVolume(_volume);
}

void ofxGstVideoSyncPlayer::initAsMaster( const std::string _clockIp, const int _clockPort, const int _oscRcvPort )
{
    if( m_initialized ){
        ofLogWarning() << " Player already initialized as MASTER with Ip : " << _clockIp << std::endl;
        return;
    }

    m_clockIp = _clockIp;
    m_clockPort = _clockPort;

    m_isMaster = true;

    m_rcvPort = _oscRcvPort;

    m_oscSender = shared_ptr<ofxOscSender>(new ofxOscSender);
    m_oscReceiver = shared_ptr<ofxOscReceiver>(new ofxOscReceiver);

    ofAddListener(m_gstPlayer->getGstVideoUtils()->eosEvent, this, &ofxGstVideoSyncPlayer::movieEnded);

    m_oscReceiver->setup(m_rcvPort);

    m_initialized = true;
}

void ofxGstVideoSyncPlayer::initAsSlave( const std::string _clockIp, const int _clockPort, const int _oscRcvPort, const int _oscSndPort )
{
    if( m_initialized ){
        ofLogWarning() << " Player already initialized as SLAVE with Ip : " << _clockIp << std::endl;
        return;
    }

    m_clockIp = _clockIp;
    m_clockPort = _clockPort;

    m_isMaster = false;

    m_rcvPort = _oscRcvPort;
    m_sndPort = _oscSndPort;

    m_oscSender = shared_ptr<ofxOscSender>(new ofxOscSender);
    m_oscReceiver = shared_ptr<ofxOscReceiver>(new ofxOscReceiver);

    m_oscReceiver->setup(m_rcvPort);
    m_oscSender->setup(m_clockIp, m_sndPort);

    m_initialized = true;
}

void ofxGstVideoSyncPlayer::loadAsync( std::string _path )
{
    m_videoPlayer.loadAsync(_path);

    ///> Now that we have loaded we can grab the pipeline..
    m_gstPipeline = m_gstPlayer->getGstVideoUtils()->getPipeline();

    ///> Since we will provide a network clock for the synchronization
    ///> we disable here the internal handling of the base time.
    gst_element_set_start_time(m_gstPipeline, GST_CLOCK_TIME_NONE);

    if( !m_isMaster ){
         ofxOscMessage m;
         m.setAddress("/client-loaded");
         m.addInt64Arg(m_rcvPort);
         if( m_oscSender ){
            m_oscSender->sendMessage(m,false);
         }
    }

}

bool ofxGstVideoSyncPlayer::load( std::string _path )
{
    bool _loaded = false;

    _loaded = m_videoPlayer.load(_path);

    if( !_loaded ){
        ofLogError() << " Failed to load video --> " << _path << std::endl;
        return false;
    }

    ///> Now that we have loaded we can grab the pipeline..
    m_gstPipeline = m_gstPlayer->getGstVideoUtils()->getPipeline();

    ///> Since we will provide a network clock for the synchronization
    ///> we disable here the internal handling of the base time.
    gst_element_set_start_time(m_gstPipeline, GST_CLOCK_TIME_NONE);

    if( m_isMaster ){
        ///> Remove any previously used clock.
        if(m_gstClock) g_object_unref((GObject*) m_gstClock);
        m_gstClock = NULL;

        ///> Grab the pipeline clock.
        m_gstClock = gst_pipeline_get_clock(GST_PIPELINE(m_gstPipeline));

        ///> Set this clock as the master network provider clock.
        ///> The slaves are going to poll this clock through the network
        ///> and adjust their times based on this clock and their local observations.
        gst_net_time_provider_new(m_gstClock, m_clockIp.c_str(), m_clockPort);
    }

    if( !m_isMaster ){
         ofxOscMessage m;
         m.setAddress("/client-loaded");
         m.addInt64Arg(m_rcvPort);
         if( m_oscSender ){
            m_oscSender->sendMessage(m,false);
         }
    }

    return _loaded;
}


void ofxGstVideoSyncPlayer::update()
{
    if( m_isMaster && m_loop && m_movieEnded ){


        ///> Get ready to start over..
        gst_element_set_state(m_gstPipeline, GST_STATE_READY);
        gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

        ///> Set the master clock i.e This the clock that the slaves will poll
        ///> in order to keep in-sync.
        setMasterClock();

        ///> ..and start playing the master..
        gst_element_set_state(m_gstPipeline, GST_STATE_PLAYING);
        gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

        ///> Let the slaves know that they should start over..
        sendLoopMsg();

        m_movieEnded = false;
        m_paused = false;
    }

    if( m_oscReceiver ){
        while( m_oscReceiver->hasWaitingMessages() ){
            ofxOscMessage m;
            m_oscReceiver->getNextMessage(&m);
            if( m.getAddress() == "/client-loaded" && m_isMaster ){

                ///> If the video loading has failed on the master there is little we can do here...
                if(!m_videoPlayer.isLoaded()){
                    ofLogError() << " ERROR: Client loaded but master NOT !! Playback wont work properly... " << std::endl;
                    return;
                }

                string _newClient = m.getRemoteIp();
                int _newClientPort = m.getArgAsInt64(0);

                ClientKey _newClientKey(m.getRemoteIp(), _newClientPort);

                std::pair<clients_iter, bool> ret;
                ret = m_connectedClients.insert(_newClientKey);
                if( ret.second == false ){
                    ofLogError() << " Client with Ip : " << _newClient << " and Port : " << _newClientPort << " already exists! PLAYBACK will NOT work properly" << std::endl;
                    return;
                }

                ///> If the master is paused when the client connects pause the client also.
                if( m_paused ){
                    sendPauseMsg();
                    return;
                }

                ofLogVerbose("ofxGstVideoSyncPlayer") << "New client connected with IP : " << _newClient << " and port : " << _newClientPort << std::endl;

                if( m_oscSender ){
                    m_oscSender->setup(_newClient, _newClientPort);
                    ofxOscMessage m;
                    m.setAddress("/client-init-time");
                    m.addInt64Arg(m_gstClockTime);
                    m.addInt64Arg(m_pos);
                    m.setRemoteEndpoint(_newClient, _newClientPort);
                    m_oscSender->sendMessage(m,false);
                }

            }
            else if( m.getAddress() == "/client-exited" && m_isMaster ){
                string _exitingClient = m.getRemoteIp();
                int _exitingClientPort = m.getArgAsInt64(0);

                ClientKey _clientExit(_exitingClient, _exitingClientPort);
                clients_iter it = m_connectedClients.find(_clientExit);

                if( it != m_connectedClients.end() ){
                    auto temp = it;
                    ofLogVerbose("ofxGstVideoSyncPlayer") << "Disconnecting client with IP : " << _exitingClient << " and port : " << _exitingClientPort << std::endl;
                    m_connectedClients.erase(temp);

                }
            }
            else if( m.getAddress() == "/client-init-time" && !m_isMaster ){
                ofLogVerbose("ofxGstVideoSyncPlayer") << " CLIENT RECEIVED MASTER INIT TIME! "  << m.getRemoteIp() <<std::endl;
                m_pos = m.getArgAsInt64(1);

                ///> Initial base time for the clients.
                ///> Set the slave network clock that is going to poll the master.
                setClientClock((GstClockTime)m.getArgAsInt64(0));

                ///> And start playing..
                gst_element_set_state(m_gstPipeline, GST_STATE_PLAYING);
                gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

                m_paused = false;
                m_movieEnded = false;

            }
            else if( m.getAddress() == "/play" && !m_isMaster ){
                ofLogVerbose("ofxGstVideoSyncPlayer") << " CLIENT ---> PLAY " << std::endl;

                ///> Set the base time of the slave network clock.
                setClientClock(m.getArgAsInt64(0));

                ///> And start playing..
                gst_element_set_state(m_gstPipeline, GST_STATE_PLAYING);
                gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

                m_paused = false;
                m_movieEnded =false;
            }
            else if( m.getAddress() == "/pause" && !m_isMaster ){
                m_pos = m.getArgAsInt64(0);
                ofLogVerbose("ofxGstVideoSyncPlayer") << " CLIENT ---> PAUSE " << std::endl;

                gst_element_set_state(m_gstPipeline, GST_STATE_PAUSED);
                gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

                ///> This needs more thinking but for now it gives acceptable results.
                ///> When we pause, we seek to the position of the master when paused was called.
                ///> If we dont do this there is a delay before the pipeline starts again i.e when hitting play() again after pause()..
                ///> I m pretty sure this can be done just by adjusting the base_time based on the position but
                ///> havent figured it out exactly yet..
                GstSeekFlags _flags = (GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);

                if( !gst_element_seek_simple (m_gstPipeline, GST_FORMAT_TIME, _flags, m_pos )) {
                        ofLogWarning () << "Pausing seek failed" << std::endl;
                }

                m_paused = true;
            }
            else if( m.getAddress() == "/loop" && !m_isMaster ){

                ofLogVerbose("ofxGstVideoSyncPlayer") << " CLIENT ---> LOOP " << std::endl;

                ///> Get ready to start over..
                gst_element_set_state(m_gstPipeline, GST_STATE_NULL);
                gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

                ///> Set the slave clock and base_time.
                setClientClock(m.getArgAsInt64(0));

                ///> and go..
                gst_element_set_state(m_gstPipeline, GST_STATE_PLAYING);
                gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

                m_movieEnded = false;
                m_paused = false;
            }
            else if( m.getAddress() == "/seek" && !m_isMaster ){

                gint64 newPosition = m.getArgAsInt64(1);
                ofLogVerbose("ofxGstVideoSyncPlayer") << " CLIENT ---> SEEK to " << ofToString(newPosition) << std::endl;

                GstSeekFlags _flags = (GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);

                if( !gst_element_seek_simple (m_gstPipeline, GST_FORMAT_TIME, _flags, newPosition )) {
                        ofLogWarning () << "Resync seek failed" << std::endl;
                }

                ///> Set the slave clock and base_time.
                setClientClock(m.getArgAsInt64(0));
            }
            else if( m.getAddress() == "/eos" && !m_isMaster ){
                ofLogVerbose("ofxGstVideoSyncPlayer") << " CLIENT ---> EOS " << std::endl;

                m_movieEnded = true;
                m_paused = true;
            }
        }
    }
    if( m_videoPlayer.isLoaded() && !isPaused() && !isMovieEnded() ){
        m_videoPlayer.update();
    }
}

void ofxGstVideoSyncPlayer::play()
{
    if( !m_isMaster || !m_paused ) return;

    setMasterClock();

    if( m_movieEnded ){
        ///> Get ready to start over..
        gst_element_set_state(m_gstPipeline, GST_STATE_READY);
        gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

        ///> ..and start playing the master..
        gst_element_set_state(m_gstPipeline, GST_STATE_PLAYING);
        gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
        sendLoopMsg();
    }
    else{
        gst_element_set_state(m_gstPipeline, GST_STATE_PLAYING);
        gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

        sendPlayMsg();
    }

    m_movieEnded = false;
    m_paused = false;

}

void ofxGstVideoSyncPlayer::seek(long int time_ms) {
  gint64 time_nanoseconds = time_ms * pow(10, 6);
  GstSeekFlags _flags = (GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);

  if (!gst_element_seek_simple(m_gstPipeline, GST_FORMAT_TIME, _flags, time_nanoseconds)) {
      ofLogWarning("failed to seek");
  } else {
    gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
    ofLogNotice("done seeking");
    sendSeekMsg(time_nanoseconds);
  }


}

gint64 ofxGstVideoSyncPlayer::getPosition() {
  gint64 pos;
  if (gst_element_query_position (m_gstPipeline, GST_FORMAT_TIME, &pos)) {
    return pos;
  } else {
    return 0;
  }
}

unsigned long int ofxGstVideoSyncPlayer::getPositionMilliseconds() {
  return getPosition() / pow(10, 6);
}

gint64 ofxGstVideoSyncPlayer::getDuration() {
  gint64 len;
  if (gst_element_query_duration(m_gstPipeline, GST_FORMAT_TIME, &len)) {
    return len;
  } else {
    return 0;
  }
}

unsigned long int ofxGstVideoSyncPlayer::getDurationMilliseconds() {
  return getDuration() / pow(10, 6);
}

void ofxGstVideoSyncPlayer::setMasterClock()
{
    if( !m_isMaster ) return;


    ///> Be explicit on which clock we are going to use.
    gst_pipeline_use_clock(GST_PIPELINE(m_gstPipeline), m_gstClock);
    gst_pipeline_set_clock(GST_PIPELINE(m_gstPipeline), m_gstClock);


    ///> Grab the base time..
    m_gstClockTime = gst_clock_get_time(m_gstClock);

    ///> and explicitely set it for this pipeline after disabling the internal handling of time.
    gst_element_set_start_time(m_gstPipeline, GST_CLOCK_TIME_NONE);
    gst_element_set_base_time(m_gstPipeline, m_gstClockTime);
}

void ofxGstVideoSyncPlayer::setClientClock( GstClockTime _baseTime )
{
    if( m_isMaster ) return;

    ///> The arrived master base_time.
    m_gstClockTime = _baseTime;

    ///> Remove any previously used clock.
    if(m_gstClock) g_object_unref((GObject*) m_gstClock);
    m_gstClock = NULL;

    ///> Create the slave network clock with an initial time.
    m_gstClock = gst_net_client_clock_new(NULL, m_clockIp.c_str(), m_clockPort, m_gstClockTime);

    ///> Be explicit.
    gst_pipeline_use_clock(GST_PIPELINE(m_gstPipeline), m_gstClock);
    gst_pipeline_set_clock(GST_PIPELINE(m_gstPipeline), m_gstClock);

    ///> Disable internal handling of time and set the base time.
    gst_element_set_start_time(m_gstPipeline, GST_CLOCK_TIME_NONE);
    gst_element_set_base_time(m_gstPipeline,  m_gstClockTime);

}

void ofxGstVideoSyncPlayer::draw( ofPoint _pos, float _width, float _height )
{
    if( !m_videoPlayer.getPixels().isAllocated() || !m_videoPlayer.getTexture().isAllocated() ) return;

    if( _width == -1 || _height == -1 ){
        m_videoPlayer.draw(_pos);
    }
    else{
        m_videoPlayer.draw(_pos, _width, _height);
    }
}

void ofxGstVideoSyncPlayer::drawSubsection( float _x, float _y, float _w, float _h, float _sx, float _sy )
{
    m_videoPlayer.getTexture().drawSubsection(_x, _y, _w, _h, _sx, _sy);
}

float ofxGstVideoSyncPlayer::getWidth()
{
    return m_videoPlayer.getWidth();
}

float ofxGstVideoSyncPlayer::getHeight()
{
    return m_videoPlayer.getHeight();
}

void ofxGstVideoSyncPlayer::pause()
{
    if( !m_isMaster ) return;

    if( !m_paused ){
        if( !m_videoPlayer.isLoaded() ){
            ofLogError() << " Cannot pause non loaded video ! " <<std::endl;
            return;
        }

        gst_element_set_state(m_gstPipeline, GST_STATE_PAUSED);
        gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

        gst_element_query_position(GST_ELEMENT(m_gstPipeline),GST_FORMAT_TIME,&m_pos);

        GstSeekFlags _flags = (GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);

        if (!gst_element_seek_simple (m_gstPipeline, GST_FORMAT_TIME, _flags, m_pos)) {
                ofLogWarning() << "Pausing seek failed" << std::endl;
        }

        m_paused = true;

        sendPauseMsg();
    }
}

void ofxGstVideoSyncPlayer::sendPauseMsg(){

    if( !m_isMaster || !m_initialized || !m_oscSender) return;

    for( auto& _client : m_connectedClients ){
        ofLogVerbose("ofxGstVideoSyncPlayer") << " Sending PAUSE to client : " << _client.first << " at port : " << _client.second << std::endl;
        m_oscSender->setup(_client.first, _client.second);
        ofxOscMessage m;
        m.setAddress("/pause");
        m.addInt64Arg(m_pos);
        m.setRemoteEndpoint(_client.first, _client.second);
        m_oscSender->sendMessage(m,false);
    }
}

void ofxGstVideoSyncPlayer::sendPlayMsg()
{
    if( !m_isMaster || !m_initialized || !m_oscSender ) return;

    for( auto& _client : m_connectedClients ){
        ofLogVerbose("ofxGstVideoSyncPlayer") << " Sending PLAY to client : " << _client.first << " at port : " << _client.second << std::endl;
        m_oscSender->setup(_client.first, _client.second);
        ofxOscMessage m;
        m.setAddress("/play");
        m.addInt64Arg(m_gstClockTime);
        m.setRemoteEndpoint(_client.first, _client.second);
        m_oscSender->sendMessage(m,false);
    }
}

void ofxGstVideoSyncPlayer::sendSeekMsg(gint64 newPosition)
{
  if( !m_isMaster || !m_initialized || !m_oscSender ) return;

  for( auto& _client : m_connectedClients ){
      ofLogVerbose("ofxGstVideoSyncPlayer") << " Sending SEEK to client : " << _client.first << " at port : " << _client.second << std::endl;
      m_oscSender->setup(_client.first, _client.second);
      ofxOscMessage m;
      m.setAddress("/seek");
      m.addInt64Arg(m_gstClockTime);
      m.addInt64Arg(newPosition);
      m.setRemoteEndpoint(_client.first, _client.second);
      m_oscSender->sendMessage(m,false);
  }
}

void ofxGstVideoSyncPlayer::sendLoopMsg()
{
    if( !m_isMaster || !m_initialized || !m_oscSender ) return;

    for( auto& _client : m_connectedClients ){
        ofLogVerbose("ofxGstVideoSyncPlayer") << " Sending LOOP to client : " << _client.first << " at port : " << _client.second << std::endl;
        m_oscSender->setup(_client.first, _client.second);
        ofxOscMessage m;
        m.setAddress("/loop");
        m.addInt64Arg(m_gstClockTime);
        m.setRemoteEndpoint(_client.first, _client.second);
        m_oscSender->sendMessage(m,false);
    }
}

void ofxGstVideoSyncPlayer::sendEosMsg()
{
    if( !m_isMaster || !m_initialized || !m_oscSender ) return;

    for( auto& _client : m_connectedClients ){
        ofLogVerbose("ofxGstVideoSyncPlayer") << " Sending EOS to client : " << _client.first << " at port : " << _client.second << std::endl;
        m_oscSender->setup(_client.first, _client.second);
        ofxOscMessage m;
        m.setAddress("/eos");
        m.setRemoteEndpoint(_client.first, _client.second);
        m_oscSender->sendMessage(m,false);
    }
}

ofTexture ofxGstVideoSyncPlayer::getTexture()
{
    return m_videoPlayer.getTexture();
}

bool ofxGstVideoSyncPlayer::isPaused()
{
    return m_paused;
}

bool ofxGstVideoSyncPlayer::isMovieEnded()
{
    return m_movieEnded;
}

bool ofxGstVideoSyncPlayer::isMaster()
{
    return m_isMaster;
}

void ofxGstVideoSyncPlayer::setPixelFormat( const ofPixelFormat & _pixelFormat )
{
    m_videoPlayer.setPixelFormat(_pixelFormat);
}

const Clients& ofxGstVideoSyncPlayer::getConnectedClients()
{
    return m_connectedClients;
}

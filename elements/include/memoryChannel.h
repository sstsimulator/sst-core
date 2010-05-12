// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _MEMORY_CHANNEL_H
#define _MEMORY_CHANNEL_H

#ifndef MEMORYCHANNEL_DBG
#define MEMORYCHANNEL_DBG 0
#endif

#define _MC_DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

#include <eventChannel.h>
#include <map>

using namespace SST;

template < typename addrT = unsigned long,
            typename dataT = unsigned long >
struct MemEvent 
{
        typedef enum { READ = 0, WRITE, INV_REQ } reqType_t;
        typedef enum { REQUEST = 0, RESPONSE, INV_MSG } msgType_t;

        typedef dataT data_t;
        typedef addrT addr_t;

        reqType_t reqType;
        msgType_t msgType;
        addr_t    addr;
        data_t    data;
};

template < typename addrT = unsigned long,
            typename cookieT = unsigned long,
            typename dataT = unsigned long >
class MemoryChannel : 
    public EventChannel< MemEvent< addrT, dataT > >
{
    public: // types

        typedef MemEvent< addrT, dataT >    event_t;
        typedef typename event_t::addr_t    addr_t;
        typedef dataT                       data_t;
        typedef cookieT                     cookie_t;

    private: // types
        typedef std::multimap< addr_t, cookie_t>            reqMap_t;
        typedef EventChannel< MemEvent< addrT, dataT > >    eventChannel_t;

    public: // functions

  virtual ~MemoryChannel() {;}
        MemoryChannel( Component& comp, Component::Params_t params,
                                                        std::string name )  :
            eventChannel_t( comp, params, name, 2 ),
            m_readReqCredit( 0 ),
            m_readRespCredit( 0 ),
            m_writeReqCredit( 0 ),
            m_writeRespCredit( 0 ),
            m_log( *new Log<>( "INFO MemoryChannel::", false ) ),
            m_dbg( *new Log< MEMORYCHANNEL_DBG >( "MemoryChannel::", false ) )
        {
            if ( params.find("debug") != params.end() ) {
                if ( params["debug"].compare("yes") == 0 ) {
                    m_dbg.enable();
                }
            }

            if ( params.find("info") != params.end() ) {
                if ( params["info"].compare("yes") == 0 ) {
                    m_log.enable();
                }
            }

            if ( params.find("readReqCredit") != params.end() ) {
                m_readReqCredit = str2long( params["readReqCredit"] ); 
            }
            if ( params.find("writeReqCredit") != params.end() ) {
                m_writeReqCredit = str2long( params["writeReqCredit"] ); 
            }
            if ( params.find("readRespCredit") != params.end() ) {
                m_readRespCredit = str2long( params["readRespCredit"] ); 
            }
            if ( params.find("writeRespCredit") != params.end() ) {
                m_writeRespCredit = str2long( params["writeRespCredit"] ); 
            }

            m_log.write( "readReqCredit=%d readRespCredit=%d\n",
                        m_readReqCredit, m_readRespCredit );
            m_log.write( "writeReqCredit=%d writeRespCredit=%d\n",
                        m_writeReqCredit, m_writeRespCredit );
        }

        virtual inline bool ready( typename event_t::msgType_t msgType,
                    typename event_t::reqType_t reqType = event_t::READ )
        {
            return eventChannel_t::ready( 
                                calcCredit( msgType, reqType ), msgType );
        }

        virtual inline bool send( event_t* event, cookie_t cookie = 0 )
        {
            return send( event, event->msgType, event->reqType,
                                                    event->addr, cookie );
        }

        virtual inline bool recv( event_t** event, cookie_t* cookie = 0 )
        {
            if ( recv( event, event_t::RESPONSE, cookie ) ) {
                return true;
            }
            return recv( event, event_t::REQUEST, cookie );
        }

        virtual inline bool send( event_t* event, 
                                    typename event_t::msgType_t msgType,
                                    typename event_t::reqType_t reqType,
                                    addr_t addr, cookie_t cookie = 0 )
        {
            _MC_DBG("event=%p cookie=%#lx type=%d:%d \n", event, cookie, 
                                msgType, reqType );

            int tokens = calcCredit( msgType, reqType );
            if ( ! eventChannel_t::ready( tokens, msgType ) ) {
                return false;
            }

            if ( cookie ) {
                storeCookie( reqType, cookie, addr );
            } 

            return eventChannel_t::send( event, tokens, msgType );
        }

        virtual inline bool recv( event_t** event,
                                        typename event_t::msgType_t type,
                                        cookie_t* cookie = 0 )
        {
            if ( ! eventChannel_t::recv( event, type ) ) {
                return false;
            }

            if ( cookie && (*event)->msgType == event_t::RESPONSE ) {
                *cookie = findCookie( (*event)->reqType, (*event)->addr );
            }

            _MC_DBG("event=%p cookie=%#lx\n", *event, cookie );
            return true;
        }

    private:
        inline int calcCredit( typename event_t::msgType_t msgType,
                                typename event_t::reqType_t reqType )
        {
            if ( msgType == event_t::REQUEST )  {
                if ( reqType == event_t::READ ) {
                    return m_readReqCredit;
                } else {
                    return m_writeReqCredit;
                }
            } else {
                if ( reqType == event_t::READ ) {
                    return m_readRespCredit;
                } else {
                    return m_writeRespCredit;
                }
            }
        } 

        inline void storeCookie( typename event_t::reqType_t type,
                                    cookie_t cookie, addr_t addr )
        {
            if ( type == event_t::WRITE )  {
                m_writeReqMap.insert( std::make_pair( addr, cookie ) );
            } else {
                m_readReqMap.insert( std::make_pair( addr, cookie ) );
            }
        }

        inline cookie_t findCookie( typename event_t::reqType_t type,
                                    addr_t addr )
        {
            typename reqMap_t::iterator iter;
            if ( type == event_t::WRITE )  {
                iter = m_writeReqMap.find( addr );
                if ( iter != m_writeReqMap.end() ) {
                    m_writeReqMap.erase(iter);
                    return iter->second;
                }
            } else {
                iter = m_readReqMap.find( addr );
                if ( iter != m_readReqMap.end() ) {
                    m_readReqMap.erase(iter);
                    return iter->second;
                }
            }
            return 0;
        }

    private: // data

        reqMap_t    m_writeReqMap;
        reqMap_t    m_readReqMap;
        int         m_readReqCredit;
        int         m_readRespCredit;
        int         m_writeReqCredit;
        int         m_writeRespCredit;

        Log<>&                      m_log;
        Log< MEMORYCHANNEL_DBG >&   m_dbg;
};

#endif

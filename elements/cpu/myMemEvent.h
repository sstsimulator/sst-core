
#ifndef _MYMEMEVENT_H
#define _MYMEMEVENT_H

#include <sst/compEvent.h>

namespace SST {

class MyMemEvent : public CompEvent {
    public:
        typedef enum { MEM_LOAD, MEM_LOAD_RESP, 
                MEM_STORE, MEM_STORE_RESP } Type_t;
        MyMemEvent() : CompEvent() { }

        unsigned long   address;
        // this should be Type_t but SERIALIZATION barfs on it
        int             type;
        uint64_t        tag;
    
    private:
	
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
	  printf("Serializing MyMemEvent\n");
	  _AR_DBG(MemEvent,"\n");
            boost::serialization::
                void_cast_register(static_cast<MyMemEvent*>(NULL), 
                                   static_cast<CompEvent*>(NULL));
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( CompEvent );
            ar & BOOST_SERIALIZATION_NVP( address );
            ar & BOOST_SERIALIZATION_NVP( type );
            ar & BOOST_SERIALIZATION_NVP( tag );
            _AR_DBG(MemEvent,"\n");
        }
};

    
} //namespace SST

#endif

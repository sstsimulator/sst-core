
#ifndef _TRIG_NIC_EVENT_H
#define _TRIG_NIC_EVENT_H

#include <sst/compEvent.h>
#include "elements/portals4_sm/trig_cpu/portals_types.h"

namespace SST {
    
    
    class trig_nic_event : public CompEvent {
    public:
	trig_nic_event() : CompEvent() {
	    portals = false;
	}

        ~trig_nic_event() {}

	int src;
        int dest;

        bool portals;
	bool head_packet;
	int stream;
        int latency; // Latency through NIC in ns
        int data_length;
        void *start;
      
	ptl_int_nic_op_type_t ptl_op;

	//         ptl_int_nic_op_t* operation;
	union {
	    ptl_int_me_t* me;
	    ptl_int_trig_op_t* trig;
	    ptl_update_ct_event_t* ct;
	    ptl_handle_ct_t ct_handle;
	    ptl_int_dma_t* dma;
	} data;

        uint32_t ptl_data[16];

    private:
	
        /*         BOOST_SERIALIZE { */
        /* 	  printf("Serializing MyMemEvent\n"); */
        /* 	  _AR_DBG(MemEvent,"\n"); */
        /*             BOOST_VOID_CAST_REGISTER( MyMemEvent*, CompEvent* ); */
        /*             ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( CompEvent ); */
        /*             ar & BOOST_SERIALIZATION_NVP( address ); */
        /*             ar & BOOST_SERIALIZATION_NVP( type ); */
        /*             ar & BOOST_SERIALIZATION_NVP( tag ); */
        /*             _AR_DBG(MemEvent,"\n"); */
        /*         } */
    };

    
} //namespace SST

#endif

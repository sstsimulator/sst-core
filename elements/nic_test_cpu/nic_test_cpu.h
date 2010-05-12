#ifndef _NIC_TEST_CPU_H
#define _NIC_TEST_CPU_H

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/link.h>
#include <sst/cpunicEvent.h>
#include "../../user_includes/netsim/netsim_internal.h"


using namespace SST;

#define DBG_NIC_TEST_CPU 1
#if DBG_NIC_TEST_CPU
#define _NIC_TEST_CPU_DBG(lvl, fmt, args...)\
    if (nic_test_cpu_debug >= lvl)   { \
         fprintf(stderr, "%d:nic_test_cpu::%s():%d: "fmt, _debug_rank, __FUNCTION__, __LINE__, ## args); \
    }
#else
#define _NIC_TEST_CPU_DBG(fmt, args...)
#endif

extern int nic_test_cpu_debug;
const char *data= "Data Data Data Data Data Data Data Data Data Data Data Data";


class nic_test_cpu : public Component {
    public:
        nic_test_cpu(ComponentId_t id, Params_t &params) :
            Component(id),
            params(params)
        {
            _NIC_TEST_CPU_DBG(1, "NIC test CPU component %lu is on rank %d\n", id, _debug_rank);

	    // Process any parameters
            Params_t::iterator it= params.begin();

	    // Defaults
	    max_rounds= 1;
	    nic_test_cpu_debug= 0;

            while (it != params.end())   {
                if (!it->first.compare("debug"))   {
		    sscanf(it->second.c_str(), "%d", &nic_test_cpu_debug);
		}
                if (!it->first.compare("max_rounds"))   {
		    sscanf(it->second.c_str(), "%d", &max_rounds);
		    _NIC_TEST_CPU_DBG(1, "Setting max_rounds to %d\n", max_rounds);
		}
                ++it;
            }

	    // Create a handler for our link to the local NIC
/* 	    NICeventHandler= new EventHandler<nic_test_cpu, bool, Time_t, Event *> */
/* 		(this, &nic_test_cpu::handle_nic_events); */

	    NICeventHandler= new EventHandler<nic_test_cpu, bool, Event *>
		(this, &nic_test_cpu::handle_nic_events);

	    nic= LinkAdd("NIC", NICeventHandler);
	    if (nic == NULL)   {
		_NIC_TEST_CPU_DBG(0, "This test CPU expects a link to a NIC named \"NIC\"\n");
		_ABORT(Nic_test_cpu, "Check the input XML file!\n");
	    } else   {
		_NIC_TEST_CPU_DBG(1, "Added a link to the local NIC\n");
	    }
        }

	// This happens after the wire-up
        int Setup() {
	    if (Id() == 0)   {
		netsim_params_t params;
		CPUNicEvent* event;

		event= new CPUNicEvent();

		params.dest= (my_rank + 1) % nranks;
		params.msgSize= strlen(data);
		params.buf= (long)const_cast<char *>(data);
		params.match_bits= 0x123456789abcdef0ll;
		params.user_data= 0x10101010a5a5a5a5ll;
		params.ignore_bits= 0xffffffff00000000ll;

		event->AttachParams(&params, static_cast<int>(sizeof(params)));
		nic->Send(event);
	    }

            return 0;
        }

        int Finish() {
            _NIC_TEST_CPU_DBG(1, "Finishing.\n");
            return 0;
        }

    private:

        nic_test_cpu(const nic_test_cpu &c);
        bool clock(Cycle_t, Time_t);
/* 	bool handle_nic_events(Time_t, Event *); */
	bool handle_nic_events(Event *);

	Event::Handler_t *NICeventHandler;

        Params_t params;
	Link *nic;
	int my_rank;
	int nranks;
	int xdim;
	int ydim;
	int max_rounds; 
	int rounds;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            _AR_DBG(nic_test_cpu, "start\n" );
            boost::serialization::
                void_cast_register(static_cast<nic_test_cpu*>(NULL), 
                                   static_cast<Component*>(NULL));
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(NICeventHandler);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(nic);
	    ar & BOOST_SERIALIZATION_NVP(my_rank);
	    ar & BOOST_SERIALIZATION_NVP(nranks);
	    ar & BOOST_SERIALIZATION_NVP(xdim);
	    ar & BOOST_SERIALIZATION_NVP(ydim);
	    ar & BOOST_SERIALIZATION_NVP(nic_test_cpu_debug);
	    ar & BOOST_SERIALIZATION_NVP(rounds);
	    ar & BOOST_SERIALIZATION_NVP(max_rounds);
            _AR_DBG(nic_test_cpu, "done\n" );
        }

        template<class Archive>
        friend void save_construct_data(Archive & ar, 
                                        const nic_test_cpu * t, 
                                        const unsigned int file_version)
        {
            _AR_DBG(nic_test_cpu, "\n");

	    ComponentId_t   id= t->_id;
            Params_t        params= t->params;

	    ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        } 

        template<class Archive>
        friend void load_construct_data(Archive & ar, 
                                        nic_test_cpu * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(nic_test_cpu, "\n");

	    ComponentId_t   id;
            Params_t        params;

	    ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);

            ::new(t)nic_test_cpu(id, params);
        } 
};

#endif

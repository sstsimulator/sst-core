#ifndef _NICMODEL_H
#define _NICMODEL_H

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/link.h>
#include "netsim_model.h"
#include "routing.h"
#include "../../user_includes/netsim/netsim_internal.h"


using namespace SST;

#define DBG_NIC_MODEL	1
#if DBG_NIC_MODEL
#define _NIC_MODEL_DBG(lvl, fmt, args...)\
    if (get_nic_model_debug() >= lvl)   { \
         printf("%d:Nicmodel::%s():%d: " fmt, _debug_rank, __FUNCTION__, __LINE__, ## args); \
    }
#else
#define _NIC_MODEL_DBG(lvl, fmt, args...)
#endif


class Nicmodel : public Component {
    public:
        Nicmodel(ComponentId_t id, Params_t& params) :
            Component(id),
            params(params)
        { 
            _NIC_MODEL_DBG(1, "NIC model component %lu is on rank %d\n", id, _debug_rank);

	    // Process any parameters
            Params_t::iterator it= params.begin();

	    // Defaults
	    nic_model_debug= 0;
	    my_rank= -1;
	    my_router= -1;
	    num_NICs= -1;
	    num_routers= -1;
	    num_ports= -1;
	    num_links= -1;
	    rcv_router_delays= 0.0;
	    rcv_msgs= 0;;
	    rcv_total_hops= 0;;

            while (it != params.end())   {
                if (!it->first.compare("debug"))   {
		    sscanf(it->second.c_str(), "%d", &nic_model_debug);
		}
                if (!it->first.compare("rank"))   {
		    sscanf(it->second.c_str(), "%d", &my_rank);
		}
                if (!it->first.compare("num_NICs"))   {
		    sscanf(it->second.c_str(), "%d", &num_NICs);
		}
                if (!it->first.compare("num_routers"))   {
		    sscanf(it->second.c_str(), "%d", &num_routers);
		}
                if (!it->first.compare("num_ports"))   {
		    sscanf(it->second.c_str(), "%d", &num_ports);
		}
                if (!it->first.compare("num_links"))   {
		    sscanf(it->second.c_str(), "%d", &num_links);
		}

                ++it;
            }

	    _NIC_MODEL_DBG(1, "My rank is %d/%d\n", my_rank, num_NICs);
	    if ((my_rank < 0) || (my_rank >= num_NICs) || (num_NICs < 1))   {
		_ABORT(Nicmodel, "Check the input XML file! You need to specify an unique rank "
		    "for each CPU and a total number of ranks.\n");
	    }

	    _NIC_MODEL_DBG(1, "num_routers %d, num_ports %d, num_links %d\n",
		num_routers, num_ports, num_links);
	    if ((num_routers < 1) || (num_ports < 1) || (num_links < 1))   {
		_ABORT(Nicmodel, "Check the input XML file! You need to specify num_routers, "
		    "num_port, and num_links in the nic_params section!\n");
	    }



	    // Create and populate the NIC/Router/Port table
	    vrinfo= init_routing(num_routers, num_NICs);
	    for (int rank= 0; rank < num_NICs; rank++)   {
		// Find the router
		sprintf(search_str, "NIC%drouter", rank);
		it= params.begin();
		while (it != params.end())   {
		    if (!it->first.compare(search_str))   {
			sscanf(it->second.c_str(), "%d", &num_item);
			NIC_table_insert_router(rank, num_item, vrinfo);
			if (rank == my_rank)   {
			    /* This is the router I'm attached to! */
			    my_router= num_item;
			    _NIC_MODEL_DBG(1, "NIC with rank %d is attached to router %d\n",
				my_rank, my_router);
			}
			break;
		    }
		    it++;
		}

		// Find the NIC
		sprintf(search_str, "NIC%dport", rank);
		it= params.begin();
		while (it != params.end())   {
		    if (!it->first.compare(search_str))   {
			sscanf(it->second.c_str(), "%d", &num_item);
			if (num_item >= num_ports)   {
			    _ABORT(Nicmodel, "Port number %d for NIC %d, larger than num_ports %d\n",
				num_item, rank, num_ports);
			}
			NIC_table_insert_port(rank, num_item, vrinfo);
			break;
		    }
		    it++;
		}
	    }

	    if (check_NIC_table(vrinfo))   {
		_ABORT(Nicmodel, "Each of the %d NICs must list its rank and the router and "
		    "port it is attached to in the common <nic_params> section!\n", num_NICs);
	    }



	    // Populate the router adjacency matrix
	    for (int link= 0; link < num_links; link++)   {
		int left_router, left_port;
		int right_router, right_port;

		// Find the left router
		sprintf(search_str, "Link%dLeftRouter", link);
		it= params.begin();
		while (it != params.end())   {
		    if (!it->first.compare(search_str))   {
			sscanf(it->second.c_str(), "%d", &left_router);
			break;
		    }
		    it++;
		}

		// Find the left port
		sprintf(search_str, "Link%dLeftPort", link);
		it= params.begin();
		while (it != params.end())   {
		    if (!it->first.compare(search_str))   {
			sscanf(it->second.c_str(), "%d", &left_port);
			break;
		    }
		    it++;
		}

		// Find the right router
		sprintf(search_str, "Link%dRightRouter", link);
		it= params.begin();
		while (it != params.end())   {
		    if (!it->first.compare(search_str))   {
			sscanf(it->second.c_str(), "%d", &right_router);
			break;
		    }
		    it++;
		}

		// Find the right port
		sprintf(search_str, "Link%dRightPort", link);
		it= params.begin();
		while (it != params.end())   {
		    if (!it->first.compare(search_str))   {
			sscanf(it->second.c_str(), "%d", &right_port);
			break;
		    }
		    it++;
		}

		Adj_Matrix_insert(link, left_router, left_port, right_router, right_port, vrinfo);

	    }

	    if (my_rank == 0 && nic_model_debug > 1)   {
		Adj_Matrix_print(vrinfo);
	    }




	    // Create a link and a handler for the cpu
	    cpuHandler= new EventHandler<Nicmodel, bool, Event *>
		(this, &Nicmodel::handle_cpu_events);

	    cpu= LinkAdd("CPU", cpuHandler);
	    if (cpu == NULL)   {
		_NIC_MODEL_DBG(0, "The NIC model expects links to the CPU and the network "
		    "named \"CPU\" and \"NETWORK\". CPU is missing!\n");
		_ABORT(Nicmodel, "Check the input XML file!\n");
	    } else   {
		_NIC_MODEL_DBG(1, "Added a link and a handler for the cpu\n");
	    }

	    // Create a link and a handler for the network
	    netHandler= new EventHandler<Nicmodel, bool, Event *>
		(this, &Nicmodel::handle_nic_events);

	    net= LinkAdd("NETWORK", netHandler);
	    if (net == NULL)   {
		_NIC_MODEL_DBG(0, "The NIC model expects links to the CPU and the network "
		    "named \"CPU\" and \"NETWORK\". NETWORK is missing!\n");
		_ABORT(Nicmodel, "Check the input XML file!\n");
	    } else   {
		_NIC_MODEL_DBG(1, "Added a link and a handler for the network\n");
	    }

	    // Create a time converter for the NIC simulator.
	    tc= registerTimeBase("1ns", true);

	    // Create a queue for unexecpted messges. Tell it to send completion
	    // events to the CPU when these messages get matched.
	    uQ= new UnexpectedQ();
	    uQ->completion_link(cpu);

	    // Same thing for posted receives.
	    pQ= new PostedQ();
	    pQ->completion_link(cpu);



	    // Generate the routing table
	    gen_routes(my_rank, my_router, nic_model_debug, vrinfo);
        }

        ~Nicmodel()   {
	    delete pQ;
	    delete uQ;
	}

	int
	get_my_rank(void)
	{
	    return my_rank;
	}

	int
	get_num_NICs(void)
	{
	    return num_NICs;
	}

	int
	get_nic_model_debug(void)
	{
	    return nic_model_debug;
	}


    private:

        Nicmodel(const Nicmodel &c);
 	bool handle_nic_events(Event *);
	bool handle_cpu_events(Event *);
	void hton_params(netsim_params_t *params);
	void insert_graph_link(const char *from, const char *to);

	Event::Handler_t *cpuHandler;
	Event::Handler_t *netHandler;

        Params_t params;
        Link *cpu;
        Link *net;
	int my_rank;
	int my_router;
	int num_NICs;
	int num_routers;
	int num_ports;
	int num_links;
	int num_item;
	char search_str[64];
	int nic_model_debug;
	PostedQ *pQ;
	UnexpectedQ *uQ;
	double rcv_router_delays;
	long long rcv_msgs;
	long long rcv_total_hops;
	void *vrinfo;

	TimeConverter *tc;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            _AR_DBG(Nicmodel,"start\n");
            boost::serialization::
                void_cast_register(static_cast<Nicmodel*>(NULL), 
                                   static_cast<Component*>(NULL));
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(cpuHandler);
	    ar & BOOST_SERIALIZATION_NVP(netHandler);
	    ar & BOOST_SERIALIZATION_NVP(cpu);
	    ar & BOOST_SERIALIZATION_NVP(net);
	    ar & BOOST_SERIALIZATION_NVP(my_rank);
	    ar & BOOST_SERIALIZATION_NVP(my_router);
	    ar & BOOST_SERIALIZATION_NVP(num_NICs);
	    ar & BOOST_SERIALIZATION_NVP(num_routers);
	    ar & BOOST_SERIALIZATION_NVP(num_ports);
	    ar & BOOST_SERIALIZATION_NVP(num_links);
	    ar & BOOST_SERIALIZATION_NVP(num_item);
	    ar & BOOST_SERIALIZATION_NVP(search_str);
	    ar & BOOST_SERIALIZATION_NVP(nic_model_debug);
	    ar & BOOST_SERIALIZATION_NVP(pQ);
	    ar & BOOST_SERIALIZATION_NVP(uQ);
	    ar & BOOST_SERIALIZATION_NVP(rcv_router_delays);
	    ar & BOOST_SERIALIZATION_NVP(rcv_msgs);
	    ar & BOOST_SERIALIZATION_NVP(rcv_total_hops);
	    ar & BOOST_SERIALIZATION_NVP(vrinfo);
	    ar & BOOST_SERIALIZATION_NVP(tc);
            _AR_DBG(Nicmodel,"done\n");
        }

        template<class Archive>
        friend void save_construct_data(Archive & ar, 
                                        const Nicmodel * t, 
                                        const unsigned int file_version)
        {
            _AR_DBG(Nicmodel,"\n");
            ComponentId_t     id     = t->_id;
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        } 

        template<class Archive>
        friend void load_construct_data(Archive & ar, 
                                        Nicmodel * t, 
                                        const unsigned int file_version)
        {
            _AR_DBG(Nicmodel,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);
            ::new(t)Nicmodel(id, params);
        } 
};

#endif

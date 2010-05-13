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


#ifndef _ROUTERMODEL_H
#define _ROUTERMODEL_H

#include <string.h>
#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/link.h>

using namespace SST;

#define DBG_ROUTER_MODEL 1
#if DBG_ROUTER_MODEL
#define _ROUTER_MODEL_DBG(lvl, fmt, args...)\
    if (router_model_debug >= lvl)   { \
	printf("%d:Routermodel::%s():%d: " fmt, _debug_rank, __FUNCTION__, __LINE__, ## args); \
    }
#else
#define _ROUTER_MODEL_DBG(lvl, fmt, args...)
#endif

extern int router_model_debug;

#define MAX_LINK_NAME		(16)


class Routermodel : public Component {
    public:
        Routermodel(ComponentId_t id, Params_t& params) :
            Component(id),
            params(params)
        { 

            Params_t::iterator it= params.begin();
	    // Defaults
	    router_model_debug= 0;
	    num_ports= -1;

            while (it != params.end())   {
                _ROUTER_MODEL_DBG(1, "Router %s: key=%s value=%s\n", component_name.c_str(),
		    it->first.c_str(), it->second.c_str());

		if (!it->first.compare("debug"))   {
		    sscanf(it->second.c_str(), "%d", &router_model_debug);
		}

		if (!it->first.compare("hop_delay"))   {
		    sscanf(it->second.c_str(), "%ld", &hop_delay);
		}

		if (!it->first.compare("component_name"))   {
		    component_name= it->second;
		    _ROUTER_MODEL_DBG(1, "Component name for ID %lu is \"%s\"\n", id, component_name.c_str());
		}

		if (!it->first.compare("num_ports"))   {
		    sscanf(it->second.c_str(), "%d", &num_ports);
		}

                ++it;
            }


	    if (num_ports < 1)   {
		_abort(Routermodel, "Need to define the num_ports parameter!\n");
	    }

	    /* Attach the handler to each port */
	    for (int i= 0; i < num_ports; i++)   {
		sprintf(link_name, "Link%dname", i);
		it= params.begin();
		while (it != params.end())   {
		    if (!it->first.compare(link_name))   {
			break;
		    }
		    it++;
		}

		if (it != params.end())   {
		    strcpy(new_port.link_name, it->second.c_str());
		    new_port.link= Routermodel::initPort(i, new_port.link_name);
		    new_port.cnt_in= 0;
		    new_port.cnt_out= 0;
		    port.push_back(new_port);
		    _ROUTER_MODEL_DBG(2, "Added handler for port %d, link \"%s\", on router %s\n",
			i, new_port.link_name, component_name.c_str());
		} else   {
		    /* Push a dummy port, so port numbering and order in list match */
		    strcpy(new_port.link_name, "Unused_port");
		    new_port.link= NULL;
		    new_port.cnt_in= 0;
		    new_port.cnt_out= 0;
		    port.push_back(new_port);
		    _ROUTER_MODEL_DBG(2, "Recorded unused port %d, link \"%s\", on router %s\n",
			i, new_port.link_name, component_name.c_str());
		}
	    }

	    _ROUTER_MODEL_DBG(1, "Router model component \"%s\" is on rank %d\n",
		component_name.c_str(), _debug_rank);

	    // Create a time converter for the NIC simulator.
	    tc= registerTimeBase("1ns", true);
        }

    private:

        Routermodel(const Routermodel &c);
	bool handle_port_events(Event *, int in_port);
	Link *initPort(int port, char *link_name);
	Event::Handler_t *RouterPortHandler;

        Params_t params;

	TimeConverter *tc;
	SimTime_t hop_delay;
	std::string component_name;

	char link_name[MAX_LINK_NAME];

	typedef struct port_t   {
	    char link_name[MAX_LINK_NAME];
	    Link *link;
	    long long int cnt_in;
	    long long int cnt_out;
	} port_t;
	std::vector<port_t> port;
	std::vector<port_t>::iterator portNum;
	port_t new_port;
	int num_ports;

	typedef struct router_t   {
	    int Id;
	    // Record how each port links to another router/port
	} router_t;
	std::vector<router_t> routers;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            _AR_DBG(Routermodel,"start\n");
            boost::serialization::
                void_cast_register(static_cast<Routermodel*>(NULL), 
                                   static_cast<Component*>(NULL));
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(port);
	    ar & BOOST_SERIALIZATION_NVP(tc);
            _AR_DBG(Routermodel,"done\n");
        }

        template<class Archive>
        friend void save_construct_data(Archive & ar, 
                                        const Routermodel * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Routermodel,"\n");
            ComponentId_t     id     = t->_id;
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        } 

        template<class Archive>
        friend void load_construct_data(Archive & ar, 
                                        Routermodel * t, 
                                        const unsigned int file_version)
        {
            _AR_DBG(Routermodel,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);
            ::new(t)Routermodel(id, params);
        } 
};

#endif

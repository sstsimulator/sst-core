#include <sst_config.h>

#include <arpa/inet.h>      // for endianness
#include <sys/param.h>      // for endianness
#include <sst/cpunicEvent.h>
#include "nicmodel.h"
#include "netsim_model.h"
#include "routing.h"

#include "../../user_includes/netsim/netsim_internal.h"



bool
// Nicmodel::handle_nic_events(Time_t time, Event *event)
Nicmodel::handle_nic_events(Event *event)
{

CPUNicEvent *e;
bool rc;


    e= static_cast<CPUNicEvent *>(event);
    _NIC_MODEL_DBG(2, "NIC %lu got an event from the NETWORK at time %lld\n",
	Id(), (long long)getCurrentSimTime());

    // Collect some stats
    rcv_router_delays += e->router_delay;
    rcv_msgs++;
    rcv_total_hops += e->hops;
    _NIC_MODEL_DBG(5, "NIC %lu: Router delay %15.9f, rcvs %lld, hops %lld\n",
	Id(), (double)e->router_delay, rcv_msgs, rcv_total_hops);

    // Is there a matching receive?
    rc= pQ->match(e);
    if (rc)   {
	// We're done. Completion event is already sent.
	_NIC_MODEL_DBG(2, "NIC %lu: Matched posted receive!\n", Id());
    } else   {
	// No match, we have to put it into the unexpected queue
	uQ->insert(e);
	_NIC_MODEL_DBG(2, "NIC %lu: Goes into unexpected queue\n", Id());
    }

    return false;
}


bool
Nicmodel::handle_cpu_events(Event *event)
{

int rc;
int *route;


    _NIC_MODEL_DBG(2, "NIC %lu got an event from the local CPU at time %lld\n",
	Id(), (long long)getCurrentSimTime());

    netsim_params_t params;
    CPUNicEvent *e;
    int len;

    e= static_cast<CPUNicEvent *>(event);
    len= sizeof(params);
    e->DetachParams(static_cast<void *>(&params), &len);
    hton_params(&params);
    _NIC_MODEL_DBG(5, "NIC %lu: Routine \"%d\" with %d bytes of data\n",
	Id(), e->GetRoutine(), len);

    switch (e->GetRoutine())   {
	case NETSIM_INIT:
	    _NIC_MODEL_DBG(1, "NIC %lu: my_rank %d, nranks %d, debug %d\n",
		Id(), my_rank, num_NICs, get_nic_model_debug());

	    // Initialize stats
	    rcv_router_delays= 0.0;
	    rcv_msgs= 0;;
	    rcv_total_hops= 0;;

	    // Send info back to CPU
	    params.my_rank= my_rank;
	    params.nranks= num_NICs;
	    params.rc= 1; // Success
	    params.type= INIT_ANSWER;
	    hton_params(&params);  // Convert back to network order
	    e->AttachParams(&params, static_cast<int>(sizeof(params)));
	    cpu->Send(e);
	    // Don't delete event
	    break;

	case NETSIM_TX_START:
	    // Set up routing
	    e->router_delay= 0.0; // No delay so far
	    e->hops= 0; // No hops so far

	    // add it to the completion queue on our CPU
	    add_snd_completion(cpu, SND_COMPLETION, NETSIM_SUCCESS, params.msgSize, params.match_bits, params.user_data);
	    e->msg_match_bits= params.match_bits;
	    e->msg_len= params.msgSize;

	    // params.msg_src= my_rank;
	    // params.msg_start= time;
	    params.rc= 1; // Success
	    params.type= SND_COMPLETION;
	    _NIC_MODEL_DBG(2, "NIC %lu is going to send %d bytes to rank %d\n",
		Id(), params.msgSize, params.dest);

	    // Attach the source route
	    route= get_route(params.dest, vrinfo);
	    while (route && (*route >= 0))   {
		e->route.push_back(*route);
		route++;
	    }

	    // Do the message send over the network
	    hton_params(&params);  // Convert back to network order
	    e->AttachParams(&params, static_cast<int>(sizeof(params)));

	    net->Send(e);

	    break;

	case NETSIM_RX_START:
	    _NIC_MODEL_DBG(2, "NIC %lu is Posting a receive for %d bytes, match 0x%016llx, "
		"ignore 0x%016llx\n",
		Id(), params.msgSize, (long long)params.match_bits, (long long)params.ignore_bits);

	    // See if the message is already in the unexpected queue
	    rc= uQ->find(params.match_bits, params.ignore_bits, params.user_data, params.msgSize, params.buf);
	    if (rc)   {
		// Yes. We're done
		_NIC_MODEL_DBG(2, "NIC %lu: Found a matching send in unexpected Queue\n", Id());
		break;
	    }

	    // Msg is not here yet; post it.
	    _NIC_MODEL_DBG(2, "NIC %lu: Posting message for later\n", Id());
	    pQ->post(params.buf, params.msgSize, params.match_bits, params.ignore_bits, params.user_data);
	    delete event;
	    break;

	case NETSIM_FINALIZE:
	    // Print some stats
	    printf("NIC %3lu received %lld messages total\n", Id(), rcv_msgs);
	    if (rcv_msgs > 0)   {
		printf("NIC %3lu total delay was %15.9f, avg %15.9f\n", Id(),
		    rcv_router_delays, rcv_router_delays / rcv_msgs);
	    } else   {
		printf("NIC %3lu total delay was %15.9f\n", Id(), rcv_router_delays);
	    }
	    if (rcv_msgs > 0)   {
		printf("NIC %3lu total hops %9lld, avg %15.9f\n", Id(),
		    rcv_total_hops, (double)rcv_total_hops / rcv_msgs);
	    } else   {
		printf("NIC %3lu total hops %9lld\n", Id(), rcv_total_hops);
	    }
	    delete event;
	    break;

	// These calls should never make it here. They are handled by the CPU in user
	// space or the kernel
	case NETSIM_SIZE:
	case NETSIM_RANK:
	case NETSIM_PROBE_START:
	case NETSIM_CQ_POLL:
	case NETSIM_GET_CLOCK:
	    delete event;
	    break;
    }

    return false;
}



//
// Private functions
//
void
Nicmodel::hton_params(netsim_params_t *params)
{

    uint64_t lower32, upper32;

    params->rc= htonl(params->rc);
    params->synchronous= htonl(params->synchronous);

    // Don't convert buf here

    lower32= htonl(params->match_bits >> 32);
    upper32= htonl(params->match_bits);
    params->match_bits= (upper32 << 32) | lower32;

    // We don't need to convert the user_data because we never look at it

    lower32= htonl(params->ignore_bits >> 32);
    upper32= htonl(params->ignore_bits);
    params->ignore_bits= (upper32 << 32) | lower32;

    params->dest= htonl(params->dest);
    params->msgSize= htonl(params->msgSize);
    params->my_rank= htonl(params->my_rank);
    params->nranks= htonl(params->nranks);
    params->type= htonl(params->type);
    params->status= htonl(params->status);

    _NIC_MODEL_DBG(5, "NIC %lu converting parameter fields\n", Id());
    _NIC_MODEL_DBG(5, "NIC %lu params->rc %d\n", Id(), params->rc);
    _NIC_MODEL_DBG(5, "NIC %lu params->synchronous %d\n", Id(), params->synchronous);
    _NIC_MODEL_DBG(5, "NIC %lu params->buf 0x%08llx (not converted)\n", Id(), (long long)params->buf);
    _NIC_MODEL_DBG(5, "NIC %lu params->match_bits 0x%016llx\n", Id(), (long long)params->match_bits);
    _NIC_MODEL_DBG(5, "NIC %lu params->user_data 0x%016llx (not converted)\n",
	Id(), (long long)params->user_data);
    _NIC_MODEL_DBG(5, "NIC %lu params->ignore_bits 0x%016llx\n", Id(), (long long)params->ignore_bits);
    _NIC_MODEL_DBG(5, "NIC %lu params->dest %d\n", Id(), params->dest);
    _NIC_MODEL_DBG(5, "NIC %lu params->msgSize %d\n", Id(), params->msgSize);
    _NIC_MODEL_DBG(5, "NIC %lu params->my_rank %d\n", Id(), params->my_rank);
    _NIC_MODEL_DBG(5, "NIC %lu params->nranks %d\n", Id(), params->nranks);
    _NIC_MODEL_DBG(5, "NIC %lu params->type %d\n", Id(), params->type);
    _NIC_MODEL_DBG(5, "NIC %lu params->status %d\n", Id(), params->status);

}  // end of hton_params()



extern "C" {
Nicmodel*
nicmodelAllocComponent(SST::ComponentId_t id,
                       SST::Component::Params_t& params)
{
    return new Nicmodel(id, params);
}
}

#if WANT_CHECKPOINT_SUPPORT

BOOST_CLASS_EXPORT(Nicmodel)

// BOOST_CLASS_EXPORT_TEMPLATE4(SST::EventHandler,
//     Nicmodel, bool, SST::Time_t, SST::Event *)

BOOST_CLASS_EXPORT_TEMPLATE3(SST::EventHandler,
    Nicmodel, bool, SST::Event *)

#endif

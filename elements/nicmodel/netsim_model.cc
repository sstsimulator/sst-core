#include <arpa/inet.h>		// For endian conversion
#include <sst/cpunicEvent.h>
#include "netsim_model.h"
#include "../../user_includes/netsim/netsim_internal.h"



//
// -----------------------------------------------------------------------------
// Posting, matching and unexpected queue handling
// -----------------------------------------------------------------------------
//
void
PostedQ::post(uint64_t buf, uint32_t msgSize, uint64_t match_bits, uint64_t ignore_bits, uint64_t user_data)
{

post_t p;


    p.buf= buf;
    p.msgSize= msgSize;
    p.match_bits= match_bits;
    p.ignore_bits= ignore_bits;
    p.user_data= user_data;

    postedQ->push_back(p);

}  // end of post()



// Search the posted queue for a match
bool
PostedQ::match(SST::CPUNicEvent *e)
{

std::list<post_t>::iterator p;
int len;


    // printf("========================== match(len %d, bits 0x%016lx)\n", e->msg_len, e->msg_match_bits);
    for (p= postedQ->begin(); p != postedQ->end(); ++p)   {
	if ((p->match_bits & ~p->ignore_bits)  == (e->msg_match_bits & ~p->ignore_bits))   {
	    if (e->msg_len > p->msgSize)   {
		// Truncate
		len= p->msgSize;
	    } else   {
		len= e->msg_len;
	    }
	    printf("========================== match() found one!\n");
	    add_rcv_completion(completion, e, RCV_COMPLETION, NETSIM_SUCCESS, len, e->msg_match_bits, p->user_data, p->buf);

	    // printf("========================== match() remove it from the posted Q!\n");
	    postedQ->erase(p);
	    return true;
	}
    }

    printf("========================== match() found nothing\n");
    return false;

}  // End of match



void
UnexpectedQ::insert(SST::CPUNicEvent *e)
{
    unexpectedQ->push_back(e);
}  // end of insert()



bool 
UnexpectedQ::find(uint64_t match_bits, uint64_t ignore_bits, uint64_t user_data, uint32_t requested_len, uint64_t buf)
{

std::list<SST::CPUNicEvent *>::iterator p;
int len;


    for (p= unexpectedQ->begin(); p != unexpectedQ->end(); ++p)   {
	if (((*p)->msg_match_bits & ~ignore_bits)  == (match_bits & ~ignore_bits))   {
	    // We have a match: Send a completion event with the length we actually
	    // received, the match bits we received, and the local user data
	    if ((*p)->msg_len > requested_len)   {
		// Truncate
		len= requested_len;
	    } else   {
		len= (*p)->msg_len;
	    }
	    // printf("========================== UnexpectedQ::find() Found a match for %d bytes\n", len);
	    add_rcv_completion(completion, *p, RCV_COMPLETION, NETSIM_SUCCESS, len, (*p)->msg_match_bits, user_data, buf);

	    unexpectedQ->erase(p);
	    return true;
	}
    }

    return false;

}  // end of find()



// FIXME: We probably don't need response. Status may not be needed either...
void
add_snd_completion(SST::Link *Qdest, int response, int status, int msg_len,
	uint64_t msg_match_bits, uint64_t msg_user_data)
{

netsim_params_t params;
uint64_t lower32, upper32;
SST::CPUNicEvent *e;


    e= new SST::CPUNicEvent();
    if (e == NULL)   {
	fprintf(stderr, "%s: Out of memory!\n", __func__);
	return;
    }

    params.status= ntohl(status);
    params.type= ntohl(response);
    params.msgSize= ntohl(msg_len);
    lower32= htonl(msg_match_bits >> 32);
    upper32= htonl(msg_match_bits);
    params.match_bits= (upper32 << 32) | lower32;
    params.user_data= msg_user_data; // Never swap user data
    params.rc= ntohl(1); // Success
    e->AttachParams(&params, static_cast<int>(sizeof(params)));
    Qdest->Send(e);

}  // End of add_snd_completion()



// FIXME: We probably don't need response. Status may not be needed either...
void
add_rcv_completion(SST::Link *Qdest, SST::CPUNicEvent *e,
	int response, int status, int msg_len,
	uint64_t msg_match_bits, uint64_t msg_user_data, uint64_t buf)
{

netsim_params_t params;
uint64_t lower32, upper32;


    params.status= ntohl(status);
    params.type= ntohl(response);
    params.msgSize= ntohl(msg_len);
    lower32= htonl(msg_match_bits >> 32);
    upper32= htonl(msg_match_bits);
    params.match_bits= (upper32 << 32) | lower32;
    params.user_data= msg_user_data; // Never swap user data
    params.rc= ntohl(1); // Success
    e->buf= buf;
    e->AttachParams(&params, static_cast<int>(sizeof(params)));
    Qdest->Send(e);

}  // End of add_rcv_completion()

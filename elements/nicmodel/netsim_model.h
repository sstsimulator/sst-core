#ifndef _NETSIM_MODEL_H
#define _NETSIM_MODEL_H
#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>
#include <sst/cpunicEvent.h>
#include <sst/link.h>



void add_snd_completion(SST::Link *Qdest, int response, int status, int msg_len,
        uint64_t msg_match_bits, uint64_t msg_user_data);

void add_rcv_completion(SST::Link *Qdest, SST::CPUNicEvent *e, int response,
	int status, int msg_len, uint64_t msg_match_bits, uint64_t msg_user_data,
	uint64_t buf);



class UnexpectedQ   {
    public:
	UnexpectedQ()   {
	    // Initialize
	    unexpectedQ= new std::list<SST::CPUNicEvent *>;
	}

	~UnexpectedQ()   {
	    delete unexpectedQ;
	}

	void completion_link(SST::Link *link)   {
	    completion= link;
	}

	bool find(uint64_t match_bits, uint64_t ignore_bits, uint64_t user_data,
		uint32_t requested_len, uint64_t buf);

	void insert(SST::CPUNicEvent *e);


    private:
	std::list<SST::CPUNicEvent *> *unexpectedQ;
	SST::Link *completion; // Where to send completion events

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
	    _AR_DBG(UnexpectedQ, "\n");
	    ar & BOOST_SERIALIZATION_NVP(unexpectedQ);
	    ar & BOOST_SERIALIZATION_NVP(completion);
	    _AR_DBG(UnexpectedQ, "\n");
	}

};  // end of class UnexpectedQ
// BOOST_CLASS_EXPORT(UnexpectedQ)



class PostedQ   {
    public:
	PostedQ()   {
	    // Initialize
	    postedQ= new std::list<post_t>;
	}

	~PostedQ()   {
	    delete postedQ;
	}

	void completion_link(SST::Link *link)   {
	    completion= link;
	}

	void post(uint64_t buf, uint32_t msgSize, uint64_t match_bits,
		uint64_t ignore_bits, uint64_t user_data);

	bool match(SST::CPUNicEvent *event);

    private:
	typedef struct post_t   {
	    uint64_t buf;
	    uint32_t msgSize;
	    uint64_t match_bits;
	    uint64_t ignore_bits;
	    uint64_t user_data;
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version )
            {
		ar & BOOST_SERIALIZATION_NVP(buf);
		ar & BOOST_SERIALIZATION_NVP(msgSize);
		ar & BOOST_SERIALIZATION_NVP(match_bits);
		ar & BOOST_SERIALIZATION_NVP(ignore_bits);
		ar & BOOST_SERIALIZATION_NVP(user_data);
	    }
	} post_t;

	std::list<post_t> *postedQ;
	SST::Link *completion; // Where to send completion events

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
	    _AR_DBG(PostedQ, "\n");
	    ar & BOOST_SERIALIZATION_NVP(postedQ);
	    ar & BOOST_SERIALIZATION_NVP(completion);
	    _AR_DBG(PostedQ, "\n");
	}

};  // end of class PostedQ


#endif // _NETSIM_MODEL_H

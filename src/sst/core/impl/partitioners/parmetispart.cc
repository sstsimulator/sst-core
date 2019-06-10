// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/impl/partitioners/parmetispart.h>

#include <sst/core/warnmacros.h>
#include <sst/core/configGraph.h>

#ifdef HAVE_PARMETIS

using namespace std;

namespace SST {
namespace IMPL {
namespace Partition {

	SSTParMETISPartition::SSTParMETISPartition( RankInfo world_size, RankInfo my_rank, int verbosity ) {
		rank = my_rank;
		rankcount = world_size;

		output = new Output("SST::Core::ParMETISPartition[@p:@l on Rank @R] ", verbosity, 0, SST::Output::STDOUT);
		output->verbose(CALL_INFO, 1, 0, "Initializing ParMETIS interface on rank %6d out of %6d.\n", rank.rank, rankcount.rank);
	}

	SSTParMETISPartition::~SSTParMETISPartition() {
		output->verbose(CALL_INFO, 1, 0, "ParMETIS Partition destructor called. Complete.\n");
		delete output;
	}

	void SSTParMETISPartition::performPartition( PartitionGraph* graph ) {
		const int my_rank = rank.rank;

		if ( 0 == my_rank ) {
			if( NULL == graph ) {
				output->fatal(CALL_INFO, -1, "Error: graph provided to the partitionner is set to NULL, internal error.\n");
			}
		}
		
		if( 0 == my_rank ) {
			output->verbose(CALL_INFO, 1, 0, "Performing partitionning of model graph...\n");
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/*
			Work out how many components we have and what a roughly even break up of these over all ranks will
			be. We want to use a roughly event split of components where possible to ensure load balancing
		*/

		const uint64_t comp_count = graph->getNumComponents();
		uint64_t* comp_id_rank_start = new uint64_t[rankcount];

		if( 0 == my_rank ) {
			uint64_t comp_per_rank = comp_count / static_cast<uint64_t>( rankcount );
			uint64_t next_comp_id_start = 0;

			output->verbose(CALL_INFO, 1, 0, "Partition info: %" PRIu64 " components, %" PRIu64 ", approx %d comp per rank\n",
				comp_count, comp_per_rank);

			for( int i = 0 ; i < comp_on_rank; ++i ) {
				comp_id_rank_start[i] = next_comp_id_start;
				next_comp_id_start += comp_per_rank;
			}
		}

		MPI_Bcast( comp_id_rank_start, rankcount, MPI_UINT64T, 0, MPI_COMM_WORLD );

		if( 0 == my_rank ) {
			output->verbose(CALL_INFO, 1, 0, "Component Count Distribution:\n");

			next_comp_id_start = 0;
			for( int i = 0; i < comp_on_rank - 1; ++i ) {
				output->verbose(CALL_INFO, 1, 0, "-> Rank %6d [%" PRIu64 ", %" PRIu64 ")\n", i, next_comp_id_start, comp_id_rank_start[i]);
				next_comp_id_start += comp_id_rank_start[i];
			}			
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		const uint64_t my_start_comp = next_comp_id_start[my_rank];
		const uint64_t my_end_comp   = ((rankcount - 1) == my_rank) ? comp_count : next_comp_id_start[my_rank + 1]);

		uint64_t local_link_count = 0;

		for( ComponentId_t i = my_start_comp; i < my_end_comp; ++i ) {
			ConfigComponent* current_comp = graph->findComponent(i);
			local_link_count += current_comp->links.size();
		}

		output->verbose(CALL_INFO, 1, 0, "Rank %6d has %15" PRIu64 " and %15" PRIu64 " links.\n", my_rank, 
			my_end_comp - my_start_comp, local_link_count);

		// Create an array of local indices and then local_links, as all edges must be (u,v) and (v,u) in ParMETIS
		idx_t* local_vertices = new idx_t[ (my_end_comp - my_start_comp) ];
		idx_t* local_links    = new idx_t[ local_link_count ];
		uint64_t next_link_index = 0;

		ConfigLinkMap_t& link_map = graph->getLinkMap();

		for( ComponentId_t i = my_start_comp; i < my_end_comp; ++i ) {
			ConfigComponent* current_comp = graph->findComponent(i);

			std::vector<LinkId_t> current_comp_links = current_comp->links;
			for( auto next_link_id = current_comp_links.begin(); next_link_id != current_comp_links.end(); next_link_id++ ) {
				if( link_map.contains( *next_link_id ) ) {
					ConfigLink& next_link = link_map[*next_link_id];
					local_links[next_link_index] = static_cast<idx_t>( next_link.component[1] );

					output->verbose(CALL_INFO, 1, 0, "Creating a link from %" PRIu64 " to %" PRIu64 "\n",
						i, next_link.component[1]);

					next_link_index++;
				} else {
					output->verbose(CALL_INFO, -1, "Link map does not contain a link with id: %" PRIu64 ", structural error in graph.\n",
						static_cast<uint64_t>( *next_link_id ) );
				}
			}
		}

		delete[] local_vertices;
		delete[] local_links;

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		delete[] comp_id_rank_start;

		if( 0 == my_rank ) {
			output-verbose(CALL_INFO, 1, 0, "Partition of model graph is complete.\n");
		}
	}
}

}
}
}

#endif

// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/impl/partitioners/zoltpart.h>

#include <sst/core/warnmacros.h>
#include <sst/core/configGraph.h>

#ifdef HAVE_ZOLTAN

using namespace std;

static SST::Output* partOutput;

namespace SST {
namespace IMPL {
namespace Partition {

extern "C" {
static int sst_zoltan_count_vertices(void* data, int* UNUSED(ierr)) {
	int rank = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(0 == rank) {
		PartitionGraph* c_graph = (PartitionGraph*) data;
		partOutput->verbose(CALL_INFO, 1, 0, "SST queried by Zoltan for partition graph vertices, found %" PRIu64 " in partition graph\n",
			(uint64_t) c_graph->getComponentMap().size());

		return c_graph->getComponentMap().size();
	} else {
		return 0;
	}
}

static void sst_zoltan_get_vertex_list(void* data, int UNUSED(sizeGID), int UNUSED(sizeLID),
	ZOLTAN_ID_PTR globalIDs, ZOLTAN_ID_PTR localIDs,
	int UNUSED(wgt_dim), float* obj_wgts, int* ierr) {

	int rank = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(0 == rank) {
		partOutput->verbose(CALL_INFO, 1, 0, "SST is queried by Zoltan for the graph vertex list, traversing graph to add to Zoltan...\n");

		PartitionGraph* c_graph = (PartitionGraph*) data;
		int localID = 0;
		int next_entry = 0;

		for(PartitionComponentMap_t::iterator comp_itr = c_graph->getComponentMap().begin();
			comp_itr != c_graph->getComponentMap().end(); comp_itr++) {
			globalIDs[next_entry] = (int) comp_itr->id;
			localIDs[next_entry] = localID++;
			obj_wgts[next_entry] = comp_itr->weight;

			next_entry++;
		}

		*ierr = ZOLTAN_OK;

		partOutput->verbose(CALL_INFO, 1, 0, "Completed traversing partition graph, vertices returned to Zoltan.\n");
		return;
	} else {
		return;
	}
}

static void sst_zoltan_get_num_edges_list(void *data, int UNUSED(sizeGID), int UNUSED(sizeLID), int num_obj,
             ZOLTAN_ID_PTR UNUSED(globalID), ZOLTAN_ID_PTR UNUSED(localID),
             int *numEdges, int *ierr) {

 	int rank = 0;
 	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

 	if(0 == rank) {
		partOutput->verbose(CALL_INFO, 1, 0, "SST queried by Zoltan for the number of edges in the graph, these will be calculated...\n");

		PartitionGraph* c_graph = (PartitionGraph*) data;
		PartitionLinkMap_t& link_map = c_graph->getLinkMap();

		// Cycle over all of the "objects" which are components
		// Then cycle over our link map, if we find this component is on the left of
		// a link add it to a counter
		// Finally let Zoltan know how many links the component has
		for(int i = 0; i < num_obj; ++i) {
			int this_comp_links = 0;
			PartitionLinkMap_t::iterator map_itr;

			for(map_itr = link_map.begin(); map_itr != link_map.end(); map_itr++) {
				if(map_itr->component[0] == (ComponentId_t) i) {
					this_comp_links++;
				}
			}

			numEdges[i] = this_comp_links;
		}

		partOutput->verbose(CALL_INFO, 1, 0, "Completed counting edges in the SST partition graph.\n");

		*ierr = ZOLTAN_OK;
		return;
	} else {
		return;
	}
}

static void sst_zoltan_get_edge_list(void *data, int UNUSED(sizeGID), int UNUSED(sizeLID),
        int num_obj, ZOLTAN_ID_PTR UNUSED(globalID), ZOLTAN_ID_PTR UNUSED(localID),
        int *num_edges,
        ZOLTAN_ID_PTR nborGID, int *nborProc,
        int UNUSED(wgt_dim), float *UNUSED(ewgts), int *ierr) {

    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, & rank);

    if(0 == rank) {
		partOutput->verbose(CALL_INFO, 1, 0, "SST is queried by Zoltan to obtain the partition graph edge list.\n");

        PartitionGraph* c_graph = (PartitionGraph*) data;
		PartitionLinkMap_t& link_map = c_graph->getLinkMap();

		if(num_obj != (int) c_graph->getComponentMap().size()) {
			fprintf(stderr, "Zoltan did not request edges for the correct number of vertices.\n");
			fprintf(stderr, "Expected request for %d vertices but got %d\n",
				(int) c_graph->getComponentMap().size(), num_obj);
			exit(-1);
		}

		ZOLTAN_ID_PTR next_nbor_entry = nborGID;
		int* next_proc_entry = nborProc;

		for(int i = 0; i < num_obj; ++i) {
			int this_comp_links = num_edges[i];
			PartitionLinkMap_t::iterator map_itr;

			for(map_itr = link_map.begin(); map_itr != link_map.end(); map_itr++) {
				if(this_comp_links < 0) {
					// Something went wrong here as we have a count of how many edges
					// we are expecting, we decrement. If this gets below 0 we got the count
					// wrong
					fprintf(stderr, "Error: Zoltan partition scheme failed. More links from a component than anticipated.\n");
				}

				if(map_itr->component[0] == (ComponentId_t) i) {
					next_nbor_entry[0] = (int) map_itr->component[1];
					next_proc_entry[0] = (int) 0;

					next_proc_entry++;
					next_nbor_entry++;
					this_comp_links--;
				}
			}
		}

		partOutput->verbose(CALL_INFO, 1, 0, "Completed copying the edge list to Zoltan.\n");

		*ierr = ZOLTAN_OK;
		return;
	} else {
		return;
	}
}
} // extern "C"

SSTZoltanPartition::SSTZoltanPartition(RankInfo world_size, RankInfo my_rank, int verbosity) {
	// Get info for MPI.
	//MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	//MPI_Comm_size(MPI_COMM_WORLD, &rankcount);
    rank = my_rank;
    rankcount = world_size;

	partOutput = new Output("SST::Core::ZoltanPart[@p:@l on Rank @R] ", verbosity, 0, SST::Output::STDOUT);

	partOutput->verbose(CALL_INFO, 1, 0, "Initializing Zoltan interface on rank %d out of %d\n", rank.rank, rankcount.rank);

	// Get Zoltan ready to partition for us
	initZoltan();
}

void SSTZoltanPartition::initZoltan() {
	partOutput->verbose(CALL_INFO, 2, 0, "Launching Zoltan initialization...\n");

	float zolt_ver = 0;
	int argc = 1;
	char* argv[1];
	argv[0] = (char*)"sstsim.x";

	int z_rc = Zoltan_Initialize(argc, argv, &zolt_ver);

	if(ZOLTAN_OK != z_rc) {
		partOutput->fatal(CALL_INFO, -1, "Error initializing the Zoltan interface to SST (return code = %d)\n", z_rc);
	} else {
		partOutput->verbose(CALL_INFO, 1, 0, "Zoltan interface was initialized successfully.\n");
	}

	partOutput->verbose(CALL_INFO, 1, 0, "Creating Zoltan configuration...\n");
	zolt_config = Zoltan_Create(MPI_COMM_WORLD);

	partOutput->verbose(CALL_INFO, 1, 0, "Created Zoltan configuration, setting parameters...\n");

	// Parameters advised in the Zoltan examples
	Zoltan_Set_Param(zolt_config, "DEBUG_LEVEL", "0");
  	Zoltan_Set_Param(zolt_config, "LB_METHOD", "GRAPH");
  	Zoltan_Set_Param(zolt_config, "LB_APPROACH", "PARTITION");
  	Zoltan_Set_Param(zolt_config, "NUM_GID_ENTRIES", "1"); 
  	Zoltan_Set_Param(zolt_config, "NUM_LID_ENTRIES", "1");
  	Zoltan_Set_Param(zolt_config, "RETURN_LISTS", "ALL");
  	Zoltan_Set_Param(zolt_config, "CHECK_GRAPH", "2"); 
  	Zoltan_Set_Param(zolt_config, "PHG_EDGE_SIZE_THRESHOLD", ".35");
  	Zoltan_Set_Param(zolt_config, "OBJ_WEIGHT_DIM", "1");

	partOutput->verbose(CALL_INFO, 1, 0, "Completed initialization of Zoltan interface.\n");
}

SSTZoltanPartition::~SSTZoltanPartition() {
	delete partOutput;
}

void SSTZoltanPartition::performPartition(PartitionGraph* graph) {


	if(0 == rank.rank) {
		assert(NULL != graph);
	}
	assert(rankcount.rank > 0);
	
	partOutput->verbose(CALL_INFO, 1, 0, "# Preparing partitioning...\n");
	
	// Register the call backs for this class
  	Zoltan_Set_Num_Obj_Fn(zolt_config, sst_zoltan_count_vertices, graph);
  	Zoltan_Set_Obj_List_Fn(zolt_config, sst_zoltan_get_vertex_list, graph);
  	Zoltan_Set_Num_Edges_Multi_Fn(zolt_config, sst_zoltan_get_num_edges_list, graph);
  	Zoltan_Set_Edge_List_Multi_Fn(zolt_config, sst_zoltan_get_edge_list, graph);
  	
  	int part_changed = 0;
  	int num_global_entries = 0;
  	int num_local_entries = 0;
  	int num_vertices_import = 0;
  	int num_vertices_export = 0;
  	
  	ZOLTAN_ID_PTR import_global_ids;
  	ZOLTAN_ID_PTR import_local_ids;
  	ZOLTAN_ID_PTR export_global_ids;
  	ZOLTAN_ID_PTR export_local_ids;
  	
  	int* import_ranks;
  	int* import_part;
  	int* export_ranks;
  	int* export_part;
  	
  	partOutput->verbose(CALL_INFO, 1, 0, "# Calling Zoltan partition...\n");
  	
  	int zolt_rc = Zoltan_LB_Partition(zolt_config, /* input (all remaining fields are output) */
        &part_changed,        /* 1 if partitioning was changed, 0 otherwise */ 
        &num_global_entries,  /* Number of integers used for a global ID */
        &num_local_entries,  /* Number of integers used for a local ID */
        &num_vertices_import,      /* Number of vertices to be sent to me */
        &import_global_ids,  /* Global IDs of vertices to be sent to me */
        &import_local_ids,   /* Local IDs of vertices to be sent to me */
        &import_ranks,    /* Process rank for source of each incoming vertex */
        &import_part,   /* New partition for each incoming vertex */
        &num_vertices_export,      /* Number of vertices I must send to other processes*/
        &export_global_ids,  /* Global IDs of the vertices I must send */
        &export_local_ids,   /* Local IDs of the vertices I must send */
        &export_ranks,    /* Process to which I send each of the vertices */
        &export_part);  /* Partition to which each vertex will belong */

  	if (zolt_rc != ZOLTAN_OK){
		partOutput->fatal(CALL_INFO, -1, "# Error using Zoltan, partition could not be formed correctly.\n");
  	} else {
  		partOutput->verbose(CALL_INFO, 1, 0, "# Zoltan partition returned successfully.\n");
  	}

  	partOutput->verbose(CALL_INFO, 1, 0, "Assigning components to ranks based on Zoltan output...\n");

	uint64_t rank_assignments[rankcount.rank];
	for(uint32_t i = 0; i < rankcount.rank; ++i) {
		rank_assignments[i] = 0;
	}

	// Go over what we have to export, set the component to the appropriate rank
	if(0 == rank.rank) {
	  	PartitionComponentMap_t& config_map = graph->getComponentMap();
		PartitionComponentMap_t::iterator config_map_itr;
		for(config_map_itr = config_map.begin(); config_map_itr != config_map.end(); config_map_itr++) {
			config_map_itr->rank = RankInfo(0,0);
		}

		partOutput->verbose(CALL_INFO, 1, 0, "Rank 0 will export %d partition graph vertices.\n", num_vertices_export);

		// Go over what we have to export, set the component to the appropriate rank
  		for(int i = 0; i < num_vertices_export; ++i) {
  			config_map[export_global_ids[i]].rank = RankInfo(export_ranks[i], 0);
			rank_assignments[export_ranks[i]]++;
  		}
  	}

  	partOutput->verbose(CALL_INFO, 1, 0, "Assignment is complete.\n");

	if(0 == rank.rank) {
		partOutput->verbose(CALL_INFO, 1, 0, "Exporting components for load balance:\n");
		for(uint32_t i = 1; i < rankcount.rank; ++i) {
			partOutput->verbose(CALL_INFO, 1, 0, "Export to rank %" PRIu32 " (assigned %" PRIu64 " components).\n", i, rank_assignments[i]);
		}
	}

  	partOutput->verbose(CALL_INFO, 1, 0, "Freeing Zoltan data structures...\n");
  	Zoltan_LB_Free_Part(&import_global_ids, &import_local_ids,
  		&import_ranks, &import_part);
  	Zoltan_LB_Free_Part(&export_global_ids, &export_local_ids,
  		&export_ranks, &export_part);
  	partOutput->verbose(CALL_INFO, 1, 0, "Partitioning is complete.\n");
}

}
}
}

#endif

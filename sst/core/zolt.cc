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


#include <sst_config.h>
#include "sst/core/serialization/core.h"

#include <boost/mpi.hpp>
#include <boost/format.hpp>
#include <boost/serialization/map.hpp>
#include <sst/core/zolt.h>
#include <sst/core/sdl.h>
#include <sst/core/debug.h>
#include <zoltan.h>

namespace mpi = boost::mpi;

/* Zoltan partitioning

   Zoltan works by having a series of callback accessors which provide
   the information on the graph. The partitioning actually occurs with
   the Zoltan_LB_Partition() function.

 */ 

namespace SST {

#define _ZOLT_DBG( fmt, args...) __DBG( DBG_ZOLT, Zolt, fmt, ## args )


/* accessors */
static Graph *theG = 0;
static int myRank = -1;

// Zoltan callback accessor
static int get_number_of_objects(void *data, int *ierr) {
  *ierr = ZOLTAN_OK;
  if (myRank == 0) {
    _ZOLT_DBG("get_number_of_objects %d\n",theG->num_vertices());
    return theG->num_vertices();
  } else 
    return 0;
}

// Zoltan callback accessor
static void get_object_list(void *data, int sizeGID, int sizeLID,
			    ZOLTAN_ID_PTR globalID, ZOLTAN_ID_PTR localID,
			    int wgt_dim, float *obj_wgts, int *ierr)
{
    if (sizeGID != 1 && wgt_dim != 1){  /* My global IDs are 1 integer */
        *ierr = ZOLTAN_FATAL;
        return;
    }

    if (myRank == 0) {
        _ZOLT_DBG("wgt_dim=%d\n",wgt_dim);
    }

	int next = 0;
    for( VertexList_t::iterator iter = theG->vlist.begin();
                                        iter != theG->vlist.end(); ++iter )
    {
        Vertex *v = (*iter).second;

    	/* application wide global ID */
    	globalID[next] = v->id();
    	/* process specific local ID  */
    	localID[next] = v->id();

		/* weight */
		obj_wgts[next] = atof( v->prop_list.get(GRAPH_WEIGHT).c_str() );

    	if (myRank == 0) {
            _ZOLT_DBG("globalId=%d localID=%d wgt=%.3f\n",
                    globalID[next],localID[next], obj_wgts[next]);
        }
		next++;
	}

    *ierr = ZOLTAN_OK;

    return;
}

// Zoltan callback accessor
static void get_num_edges_list(void *data, int sizeGID, int sizeLID,
                               int num_obj,
                               ZOLTAN_ID_PTR globalID, ZOLTAN_ID_PTR localID,
                               int *numEdges, int *ierr)
{
  int i;

    if ( (sizeGID != 1) || (sizeLID != 1)){
        *ierr = ZOLTAN_FATAL;
        return;
    }

	for (i=0;  i < num_obj ; i++){
    	int idx = globalID[i];
		Vertex *v = theG->vlist[idx];
		numEdges[i] = v->adj_list.size();
    
        if (myRank == 0 ) {
		    _ZOLT_DBG("vert %s has %d edges\n",
                    v->prop_list.get(GRAPH_COMP_NAME).c_str(),numEdges[i]);
        }
	}

    *ierr = ZOLTAN_OK;
    return;
}

// Zoltan callback accessor
static void get_edge_list(
        void *data,
        int sizeGID,
        int sizeLID,
        int num_obj,
        ZOLTAN_ID_PTR globalID,
        ZOLTAN_ID_PTR localID,
        int *num_edges, 
        ZOLTAN_ID_PTR nborGID,
        int *nborProc,
        int wgt_dim,
        float *ewgts,
        int *ierr)
{
    ZOLTAN_ID_PTR nextID = nborGID;
    int *nextProc = nborProc;
    float *nextWght = ewgts;
  
    if ( (sizeGID != 1) || (sizeLID != 1) || (wgt_dim != 1) ) {      
        *ierr = ZOLTAN_FATAL;
        return;
    }

    _ZOLT_DBG("num_obj=%d wgt_dim=%d\n", num_obj, wgt_dim);

	for (int i=0;  i < num_obj ; i++){

        _ZOLT_DBG("globalID[%d]=%d\n",i,globalID[i]);

    	int edge_count = 0;
		Vertex *v = theG->vlist[globalID[i]];
		for ( std::list<int>::iterator iter = v->adj_list.begin();
                                    iter!= v->adj_list.end(); ++iter )
        {
			++edge_count; 

			Edge *e = theG->elist[(*iter)]; 

			int neigh = globalID[i] == e->v(0) ? e->v(1) : e->v(0);

            *nextID++ = neigh;
            *nextProc++ = 0; // initial node assignment
            float tmp = 1.0 / atof(e->prop_list.get(GRAPH_WEIGHT).c_str());
			*nextWght++ = tmp;
			_ZOLT_DBG(" %d (%.9f)\n", neigh, tmp );
            
		} 
    	_ZOLT_DBG("\n");

		if (num_edges[i] != edge_count) {
			_ZOLT_DBG(" Edge count failed line %d\n", __LINE__);
			*ierr = ZOLTAN_FATAL;
			return;
		}
	}

    *ierr = ZOLTAN_OK;

    return;
}

// partition the configuation graph. Takes a Boost graph, registers
// Zoltan accessors, and then uses zoltan to partition.
void partitionGraph(Graph &config_graph, int argc, char* argv[]) {
  _ZOLT_DBG("Partitioning\n");
  theG = &config_graph;
  mpi::communicator world;
  myRank = world.rank();

  float ver;
  // int rc = Zoltan_Initialize(argc, argv, &ver);
  int rc = Zoltan_Initialize(0,NULL,&ver);
  if (rc != ZOLTAN_OK){
    printf("sorry...\n");
    MPI_Finalize();
    exit(0);
  }

  // create zoltan object
  struct Zoltan_Struct *zz = Zoltan_Create(MPI_COMM_WORLD);
    _ZOLT_DBG("\n");

  // set parameters
  Zoltan_Set_Param(zz, "DEBUG_LEVEL", "0");
  Zoltan_Set_Param(zz, "LB_METHOD", "GRAPH");
  Zoltan_Set_Param(zz, "LB_APPROACH", "PARTITION");
  // # integers to make a GID/LID name
  Zoltan_Set_Param(zz, "NUM_GID_ENTRIES", "1");  
  Zoltan_Set_Param(zz, "NUM_LID_ENTRIES", "1");
  // number of weights per object
  Zoltan_Set_Param(zz, "OBJ_WEIGHT_DIM", "1");
  // number of weights per edge
  Zoltan_Set_Param(zz, "EDGE_WEIGHT_DIM", "1");
  // return the new process and partition assignment of every local object
  Zoltan_Set_Param(zz, "RETURN_LISTS", "ALL");
  Zoltan_Set_Param(zz, "PHG_EDGE_SIZE_THRESHOLD", "1.0");

  // start partition 'from scratch'
#if 0 
  // multilevel Kernighan-Lin partitioning  
  Zoltan_Set_Param(zz, "PARMETIS_METHOD", "PARTKWAY"); 
  // serial partitioning algorithm
  Zoltan_Set_Param(zz, "PARMETIS_COARSE_ALG", "1");
  // full checking
  Zoltan_Set_Param(zz, "CHECK_GRAPH", "2"); 
  // node weight imbalance tolerance
  Zoltan_Set_Param(zz, "IMBALANCE_TOL", "1.5");
#endif

  // set call backs for zoltan
  Zoltan_Set_Num_Obj_Fn(zz, get_number_of_objects, NULL);
  Zoltan_Set_Obj_List_Fn(zz, get_object_list, NULL);
  Zoltan_Set_Num_Edges_Multi_Fn(zz, get_num_edges_list, NULL);
  Zoltan_Set_Edge_List_Multi_Fn(zz, get_edge_list, NULL);

  // Do the actual partitioning. 
  int changed, numGidEntries, numLidEntries, numImport, numExport;
  ZOLTAN_ID_PTR importGlobalGids, importLocalGids, exportGlobalGids, 
    exportLocalGids;
  int *importProcs, *importToPart, *exportProcs, *exportToPart;
  _ZOLT_DBG("starting partition\n");
  rc = Zoltan_LB_Partition(zz, /* input (all remaining fields are output) */
                           /* 1 if partitioning was changed, 0 otherwise */ 
                           &changed, 
                           /* Number of integers used for a global ID */
                           &numGidEntries, 
                           /* Number of integers used for a local ID */
                           &numLidEntries, 
                           /* Number of vertices to be sent to me */
                           &numImport,     
                           /* Global IDs of vertices to be sent to me */
                           &importGlobalGids, 
                           /* Local IDs of vertices to be sent to me */
                           &importLocalGids,
                           /* Process rank for source of each
                              incoming vertex */  
                           &importProcs,    
                           /* New partition for each incoming vertex */
                           &importToPart,   
                           /* Number of vertices I must send to
                              other processes*/
                           &numExport,      
                           /* Global IDs of the vertices I must send */
                           &exportGlobalGids,  
                           /* Local IDs of the vertices I must send */
                           &exportLocalGids,   
                           /* Process to which I send each of the vertices */
                           &exportProcs,   
                           /* Partition to which each vertex will belong */
                           &exportToPart); 
  
  if (rc != ZOLTAN_OK){
    printf("sorry...\n");
    MPI_Finalize();
    Zoltan_Destroy(&zz);
    exit(-1);
  }

  _ZOLT_DBG("changed=%d numImport=%d numExport=%d\n",
                                                changed,numImport,numExport);
  //print out
  std::map<int, int> assignmentMap;
  if (world.rank() == 0) {
    for (int i = 0; i < numExport; ++i) {
      _ZOLT_DBG("obj %d to rank %d\n", exportGlobalGids[i], exportProcs[i]);
      Vertex *v = theG->vlist[exportGlobalGids[i]]; 
      // note: graph nodes assigned to rank 0 are not put in the
      // assignment map. (default = 0)
      assignmentMap[v->id()] = exportProcs[i];
    }
  }

  // Distrubute subgraphs to each node
  broadcast(world, assignmentMap, 0);
  
  for( VertexList_t::iterator iter = theG->vlist.begin(); iter != theG->vlist.end(); ++iter ) {
	Vertex *v = (*iter).second;	
 	v->prop_list.set(GRAPH_RANK, boost::str(boost::format("%1%") % assignmentMap[ v->id() ] ) );
	v->rank = assignmentMap[ v->id() ];
  }


  // cleanup
  Zoltan_LB_Free_Part(&importGlobalGids, &importLocalGids, 
                      &importProcs, &importToPart);
  Zoltan_LB_Free_Part(&exportGlobalGids, &exportLocalGids, 
                      &exportProcs, &exportToPart);
  Zoltan_Destroy(&zz);
}

} // namespace SST

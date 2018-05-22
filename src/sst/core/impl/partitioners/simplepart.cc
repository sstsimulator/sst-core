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
#include "sst/core/impl/partitioners/simplepart.h"

#include <sst/core/warnmacros.h>

#include <string>
#include <vector>
#include <map>
#include <stdlib.h>

#include "sst/core/configGraph.h"

using namespace std;

namespace SST {
namespace IMPL {
namespace Partition {

SimplePartitioner::SimplePartitioner(RankInfo total_ranks, RankInfo UNUSED(my_rank), int UNUSED(verbosity)) :
        SSTPartitioner(),
        world_size(total_ranks),
        total_parts(world_size.rank * world_size.thread)
    {}

    SimplePartitioner::SimplePartitioner() :
        SSTPartitioner(),
        world_size(RankInfo(1,1)),
        total_parts(world_size.rank * world_size.thread)
    {}
    
    static inline int pow2(int step) {
		int value = 1;

		for(int i = 0; i < step; i++) {
			value *= 2;
		}

		return value;
	}

	// Find the index of a specific component in this array
	static inline int findIndex(ComponentId_t* theArray, const int length, ComponentId_t findThis) {
		int index = -1;
		
		for(int i = 0; i < length; i++) {
			if(theArray[i] == findThis) {
				index = i; break;
			}
		}

		return index;
	}

	// Cost up all of the links between two sets (that is all links which originate in A
	// and connect to a vertex in B
	static SimTime_t cost_external_links(ComponentId_t* setA, 
				const int lengthA,
				ComponentId_t* setB,
				const int lengthB,
				map<ComponentId_t, map<ComponentId_t, SimTime_t>*>& timeTable) {

		SimTime_t cost = 0;

		for(int i = 0; i < lengthA; i++) {
			map<ComponentId_t, SimTime_t>* compMap = timeTable[setA[i]];

			for(map<ComponentId_t, SimTime_t>::const_iterator compMapItr = compMap->begin();
				compMapItr != compMap->end();
				compMapItr++) {

				if(findIndex(setB, lengthB, compMapItr->first) > -1) {
					cost += compMapItr->second;
				}
			}
		}

		return cost;
	}

	// Perform one step of the recursive algorithm to partition the graph
	void SimplePartitioner::simple_partition_step(PartitionComponentMap_t& component_map,
			ComponentId_t* setA, const int lengthA, int rankA,
			ComponentId_t* setB, const int lengthB, int rankB,
			map<ComponentId_t, map<ComponentId_t, SimTime_t>*> timeTable,
			int step) {

		SimTime_t costExt = cost_external_links(setA, lengthA, setB, lengthB, timeTable);

		for(int i = 0; i < lengthA; i++) {
			for(int j = 0; j < lengthB; j++) {
				ComponentId_t tempA = setA[i];
				setA[i] = setB[j]; 
				setB[j] = tempA;

				SimTime_t newCost = cost_external_links(setA, lengthA, setB, lengthB, timeTable);

				// check higher? if yes then keep otherwise swap back 
				if(newCost >= costExt) {
					costExt = newCost;
				} else {
					ComponentId_t tempB = setB[j];
					setB[j] = setA[i];
					setA[i] = tempB;
				}
			}	
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Sub-divide and repeat
		for(int i = 0; i < lengthA; i++) {
			component_map[setA[i]].rank = convertPartNum(rankA);
		}

		for(int i = 0; i < lengthB; i++) {
			component_map[setB[i]].rank = convertPartNum(rankB);
		}

		const uint32_t A1_rank = rankA;
		const uint32_t A2_rank = rankA + pow2(step);

		if(A2_rank < total_parts) {
			const int lengthA1 = lengthA % 2 == 1 ? (lengthA / 2) + 1 : (lengthA / 2);
			const int lengthA2 = lengthA / 2;

			ComponentId_t* setA1 = (ComponentId_t*) malloc(sizeof(ComponentId_t) * lengthA1);
			ComponentId_t* setA2 = (ComponentId_t*) malloc(sizeof(ComponentId_t) * lengthA2);

			int A1index = 0;
			int A2index = 0;

			for(int i = 0; i < lengthA; i++) {
				if(i % 2 == 0) {
					setA1[A1index++] = setA[i];
				} else {
					setA2[A2index++] = setA[i];
				}
			}

			simple_partition_step(component_map, setA1, lengthA1, A1_rank,
				setA2, lengthA2, A2_rank, timeTable, step + 1);

			free(setA1);
			free(setA2);
		}

		const uint32_t B1_rank = rankB;
		const uint32_t B2_rank = rankB + pow2(step);

		if(B2_rank < total_parts) {
			const int lengthB1 = lengthB % 2 == 1 ? (lengthB / 2) + 1 : (lengthB / 2);
			const int lengthB2 = lengthB / 2;

			ComponentId_t* setB1 = (ComponentId_t*) malloc(sizeof(ComponentId_t) * lengthB1);
			ComponentId_t* setB2 = (ComponentId_t*) malloc(sizeof(ComponentId_t) * lengthB2);

			int B1index = 0;
			int B2index = 0;

			for(int i = 0; i < lengthB; i++) {
				if(i % 2 == 0) {
					setB1[B1index++] = setB[i];
				} else {
					setB2[B2index++] = setB[i];
				}
			}

			simple_partition_step(component_map, setB1, lengthB1, B1_rank,
				setB2, lengthB2, B2_rank, timeTable, step + 1);

			free(setB1);
			free(setB2);
		}
	}

    void
    SimplePartitioner::performPartition(PartitionGraph* graph) {
		PartitionComponentMap_t& component_map = graph->getComponentMap();


		if(total_parts == 1) {
			for(PartitionComponentMap_t::iterator compItr = component_map.begin();
				compItr != component_map.end();
				++compItr) {

				compItr->rank = RankInfo(0,0);
			}
		} else {

			// const int A_size = component_map.size() % 2 == 1 ? (component_map.size() / 2) + 1 : (component_map.size() / 2);
			// const int B_size = component_map.size() / 2;
			const int A_size = graph->getNumComponents() % 2 == 1 ? (graph->getNumComponents() / 2) + 1 : (graph->getNumComponents() / 2);
			const int B_size = graph->getNumComponents() / 2;

			ComponentId_t setA[A_size];
			ComponentId_t setB[B_size];

			int indexA = 0;
			int indexB = 0;
			int count  = 0;

			map<ComponentId_t, map<ComponentId_t, SimTime_t>*> timeTable;

			// size_t nComp = component_map.size();
			// for(size_t theComponent = 0 ; theComponent < nComp ; theComponent++ ) {
			for( PartitionComponentMap_t::iterator compItr = component_map.begin();
                 compItr != component_map.end();
                 ++compItr) {

                ComponentId_t theComponent = (*compItr).id;

				map<ComponentId_t, SimTime_t>* compConnectMap = new map<ComponentId_t, SimTime_t>();
				timeTable[theComponent] = compConnectMap;

				if(count++ % 2 == 0) {
					setA[indexA++] = theComponent;
				} else {
					setB[indexB++] = theComponent;
				}

				// vector<string> component_links = (*compItr).links;
				LinkIdMap_t component_links = (*compItr).links;

                PartitionLinkMap_t& linkMap = graph->getLinkMap();

				for(LinkIdMap_t::const_iterator linkItr = component_links.begin();
					linkItr != component_links.end();
					linkItr++) {

					// ConfigLink* theLink = (*linkItr);
					PartitionLink& theLink = linkMap[*linkItr];
					compConnectMap->insert( pair<ComponentId_t, SimTime_t>(theLink.component[1], theLink.getMinLatency()) );
				}
			}

			simple_partition_step(component_map, setA, A_size, 0, setB, B_size, 1, timeTable, 1);
		}
	}
} // namespace partition
} // namespace IMPL
} // namespace SST

// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include <sst/core/debug.h>
#include <sst/core/timeLord.h>
#include <sst/core/interfaces/coreTestMem.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>

#include "sst/core/testElements/coreTest_TracerComponent.h"

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;
using namespace SST::CoreTestTracerComponent;

/*
  USING debug = 1 to print all status messages,
        debug = 8 to print all events details.
*/

// Constructor
coreTestTracerComponent::coreTestTracerComponent( ComponentId_t id, Params& params ): Component( id ) {

    //Get Input parameters
    unsigned int debug = params.find_integer("debug", 0);
    out = new Output("coreTestTracer[@f:@l:@p] ", debug, 0, Output::STDOUT);
    out->debug(CALL_INFO, 1, 0, "Debugging set at %d Level\n", debug);

    stats = params.find_integer("statistics", 0);
    out->debug(CALL_INFO, 1, 0, "statistics and histogram reporting is %s\n", (stats ? "enabled" : "disabled"));

    pageSize = params.find_integer("pageSize", 4096);
    out->debug(CALL_INFO, 1, 0, "Address histogram bins are multiples of %d\n", pageSize);

    accessLatBins = params.find_integer("accessLatencyBins", 10);
    out->debug(CALL_INFO, 1, 0, "Number of access latency bins set to %d\n", accessLatBins);

    string frequency = params.find_string("clock", "1 Ghz");
    out->debug(CALL_INFO, 1, 0, "Registering coreTestTracer clock at %s\n", frequency.c_str());
    registerClock( frequency, new Clock::Handler<coreTestTracerComponent>(this, &coreTestTracerComponent::clock) );
    out->debug(CALL_INFO, 1, 0, "Clock registered\n");

    string tracePrefix = params.find_string("tracePrefix", "");
    if("" == tracePrefix){
        out->debug(CALL_INFO, 1, 0, "Tracing Not Enabled.\n");
        writeTrace = false;
    } else {
        out->debug(CALL_INFO, 1, 0, "Tracing is Enabled, prefix is set to %s\n", tracePrefix.c_str());
        char* traceFilePath = (char*) malloc( sizeof(char) * (tracePrefix.size()+ 20) );
        sprintf(traceFilePath, "%s", tracePrefix.c_str());
        out->output("Writing trace to file: %s\n", traceFilePath);
        traceFile = fopen(traceFilePath, "wt");
        free(traceFilePath);
        writeTrace = true;
    }

    string statsPrefix = params.find_string("statsPrefix", "");
    if("" == statsPrefix){
        out->debug(CALL_INFO, 1, 0, "Stats Not directed to file.\n");
        writeStats = false;
    } else {
        out->debug(CALL_INFO, 1, 0, "Stats are directed to file %s\n", statsPrefix.c_str());
        char* statFilePath = (char*) malloc( sizeof(char) * (statsPrefix.size()+20) );
        sprintf(statFilePath, "%s", statsPrefix.c_str());
        out->output("Writing stats to file: %s\n", statFilePath);
        statsFile = fopen(statFilePath,"wt");
        free(statFilePath);
        writeStats = true;
    }

    //flags
    writeDebug_8 = false;
    if (debug >= 8) { writeDebug_8 = true; }

    // check links
    northBus = configureLink("northBus");
    southBus = configureLink("southBus");

    picoTimeConv = SST::Simulation::getSimulation()->getTimeLord()->getTimeConverter("1ps");
    nanoTimeConv = SST::Simulation::getSimulation()->getTimeLord()->getTimeConverter("1ns");

    out->debug(CALL_INFO, 1, 0, "coreTestTracer initialization complete\n");
    nbCount = 0;
    sbCount = 0;
    timestamp = 0;

} // constructor

// destructor
coreTestTracerComponent::~coreTestTracerComponent() {}

bool coreTestTracerComponent::clock(Cycle_t current){
    timestamp++;

    unsigned int pageNum = 0;
    unsigned int accessLatency = 0;
    SST::Event *ev = NULL;
    SST::MemHierarchy::Addr addr =0;
    //uint64_t picoseconds = (uint64_t) picoTimeConv->convertFromCoreTime(Simulation::getSimulation()->getCurrentSimCycle());
    uint64_t nanoseconds = (uint64_t) nanoTimeConv->convertFromCoreTime(Simulation::getSimulation()->getCurrentSimCycle());

    // process Memevents from north-side to south-side
    while((ev = northBus->recv())){
        MemEvent *me = dynamic_cast<MemEvent*>(ev);
        if (me == NULL){
             _abort(coreTestTracerComponent::clock, "\ncoreTestTracer received bad event.\n");
        }
        addr = me->getAddr();
        nbCount++;

        // Append address info into Histogram
        pageNum = addr/pageSize;
        if(pageNum >= AddrHist.size()) {
             AddrHist.resize(pageNum + 100);
        }
        AddrHist[pageNum]+=1;
        // For this request, record its ID & current_time to calculate access-latency when response arrives in nanoseconds intervals
        //InFlightReqQueue[me->getID()] = timestamp;
        InFlightReqQueue[me->getID()] = nanoseconds;

        if(writeDebug_8 & writeTrace){
             fprintf(traceFile,"NB: Addr: 0x%" PRIu64, addr);
             fprintf(traceFile, " timestamp: %" PRIu64, timestamp);
             fprintf(traceFile, " Cmd: %u", me->getCmd());
             fprintf(traceFile, " ID: %" PRIu64 "-%d", me->getID().first, me->getID().second);
             fprintf(traceFile, " ResponseID: %" PRIu64 "-%d", me->getResponseToID().first, me->getResponseToID().second);
             //fprintf(traceFile, " @%" PRIu64, picoseconds);
             fprintf(traceFile, " @%" PRIu64 " ns", nanoseconds);
             fprintf(traceFile, "\n");
        }

        // Send the request to south-bus
        southBus->send(me);
    }

    // process events from south-side to north-side
    while((ev = southBus->recv())){
        MemEvent *me = dynamic_cast<MemEvent*>(ev);
        if (me == NULL){
            _abort(coreTestTracerComponent::clock, "\ncoreTestTracer received bad event\n");
        }
        addr = me->getAddr();
        sbCount++;

        // Do NOT Append address info into Histogram, avoid duplication of addresses,
        // address added to histogram only for NorthBus to SouthBus travel and NOT for
        // SouthBus to NorthBus response.
        /*
        pageNum = addr/pageSize;
        if(pageNum >= AddrHist.size()) { AddrHist.resize(pageNum + 100); }
        AddrHist[pageNum]+= 1;
        */

        if(InFlightReqQueue.find(me->getResponseToID()) != InFlightReqQueue.end()){
           //accessLatency = timestamp - InFlightReqQueue[me->getResponseToID()];
           accessLatency = nanoseconds - (InFlightReqQueue[me->getResponseToID()]);
           if(accessLatency >= AccessLatencyDist.size()) {
               AccessLatencyDist.resize(accessLatency+100);
           }
           AccessLatencyDist[accessLatency] += 1;
           InFlightReqQueue.erase(me->getResponseToID());
        }

        if(writeDebug_8 & writeTrace){
             fprintf(traceFile,"SB: Addr: 0x%" PRIu64, me->getAddr());
             fprintf(traceFile, " timestamp: %" PRIu64, timestamp);
             fprintf(traceFile, " Cmd: %u", me->getCmd());
             fprintf(traceFile, " ID: %" PRIu64 "-%d", me->getID().first, me->getID().second);
             fprintf(traceFile, " ResponseID: %" PRIu64 "-%d", me->getResponseToID().first, me->getResponseToID().second);
             //fprintf(traceFile, " @%" PRIu64, picoseconds);
             fprintf(traceFile, " @%" PRIu64 " ns", nanoseconds);
             fprintf(traceFile, "\n");
        }

       // Send the request to north-bus
        northBus->send(me);
    }

    return false;
} //clock

void coreTestTracerComponent::finish(){
    if(stats){
        if(writeStats){
           FinalStats(statsFile, accessLatBins);
           fclose(statsFile);
        } else {
           FinalStats(stdout, accessLatBins);
        }
    } // if stats()
    if(writeTrace){
       fclose(traceFile);
    }
} // finish()


void coreTestTracerComponent::FinalStats(FILE *fp, unsigned int numBins){
    // print stats
    fprintf(fp, "FINAL STATS:\n");
    fprintf(fp, "-----------------------------------------------------------------\n");
    fprintf(fp, "- Events at NorthBus                 : %u\n", nbCount);
    fprintf(fp, "- Events at SouthBus                 : %u\n", sbCount);
    fprintf(fp, "- Total Events                       : %u\n", nbCount+sbCount);
    fprintf(fp, "-----------------------------------------------------------------\n\n");
    //fprintf(fp, "Additional Stats:\n");
    //fprintf(fp, "- InFlightReqQueue Size              : %" PRIu64 "\n", InFlightReqQueue.size() );
    PrintAddrHistogram(fp, AddrHist);
    PrintAccessLatencyDistribution(fp, numBins);
}

void coreTestTracerComponent::PrintAddrHistogram(FILE *fp, vector<SST::MemHierarchy::Addr> bucketList){
    unsigned int count = 0;
    fprintf(fp, "Address Histogram:\n");
    fprintf(fp, "-----------------------------------------------------------------\n");
    fprintf(fp, "Address_Range: Count\n");
    for (unsigned int i=0; i<bucketList.size(); i++){
        if(bucketList.at(i) > 0){
            fprintf(fp, "- [%u-%u]: %" PRIu64 "\n", (i*pageSize),(((i+1)*pageSize)-1), bucketList.at(i));
            count += bucketList.at(i);
        }
    }
    fprintf(fp, "-----------------------------------------------------------------\n");
    fprintf(fp, "- Total_Events_Address: %u\n", count);
    fprintf(fp, "-----------------------------------------------------------------\n\n");
}

void coreTestTracerComponent::PrintAccessLatencyDistribution(FILE* fp, unsigned int numBins){
// Prints Access Latency Distribution
    unsigned int count = 0;
    unsigned int minLat = 0;
    unsigned int maxLat = 0;
    bool minSet = false;
    for (unsigned int i=0; i<AccessLatencyDist.size(); i++){
        if (AccessLatencyDist[i] > 0) {
            if(!minSet) {
               minLat = i;
               maxLat = i;
               minSet = true;
            }
            if (i > maxLat){
               maxLat = i;
            }
            count += AccessLatencyDist[i];
        }
    } // for()

    fprintf(fp, "Access Latency Distribution (ns):\n");
    fprintf(fp, "-----------------------------------------------------------------\n");
    fprintf(fp, "Min-Latency(ns): %u  Max-Latency(ns): %u  #Bins: %u\n", minLat, maxLat, numBins);
    fprintf(fp, "-----------------------------------------------------------------\n");
    fprintf(fp, "Latency Range(ns): Count\n");

    if (maxLat == minLat){
        fprintf(fp, "- [%d-%d]: %d\n", minLat, maxLat, count);
    }
    else {
        vector<unsigned int> latencyHist;
        if (0 == latencyHist.size()){
             latencyHist.resize(numBins);
        }
        float steps = (float) maxLat/numBins;
        unsigned int step = (unsigned int) ceil(steps);
        //fprintf(fp, "steps = %f\t step = %u\n", steps, step);
        for (unsigned int i=0; i<AccessLatencyDist.size(); i++){
            if(AccessLatencyDist[i] > 0) {
                unsigned int binNum = i/step;
                latencyHist[binNum] += AccessLatencyDist[i];
            }
        }
        for (unsigned int i=0; i<latencyHist.size(); i++) {
            fprintf(fp, "- [%d-%d]: %d\n", i*step, (i+1)*step-1, latencyHist[i]);
        }
    }

    fprintf(fp, "-----------------------------------------------------------------\n");
    fprintf(fp, "- Total_Events_Latency: %u\n", count);
    fprintf(fp, "-----------------------------------------------------------------\n\n");
}


// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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

#include "sst/core/testElements/coreTest_SharedObjectComponent.h"

#include "sst/core/params.h"

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace SST {
namespace CoreTestSharedObjectsComponent {

using namespace Shared;

coreTestSharedObjectsComponent::coreTestSharedObjectsComponent(SST::ComponentId_t id, SST::Params& params) :
    Component(id),
    test_array(false),
    test_map(false),
    test_set(false),
    count(0),
    check(true),
    late_write(false),
    pub(true),
    late_initialize(false)
{
    char buffer[128] = { 0 };
    snprintf(buffer, 128, "SharedObjectsComponent %3" PRIu64 "  [@t]  ", id);
    out.init(buffer, 0, 0, Output::STDOUT);

    std::string obj_type = params.find<std::string>("object_type", "array");
    if ( obj_type == "array" ) { test_array = true; }
    else if ( obj_type == "map" ) {
        test_map = true;
    }
    else if ( obj_type == "set" ) {
        test_set = true;
    }

    myid = params.find<int>("myid", -1);
    if ( myid == -1 ) { out.fatal(CALL_INFO, 1, "ERROR: myid is a required parameter\n"); }

    num_entities = params.find<int>("num_entities", 12);

    bool full_initialization = params.find<bool>("full_initializion", "true");

    bool multiple_initializers = params.find<bool>("multiple_initializers", "false");

    bool conflicting_write = params.find<bool>("conflicting_write", "false");

    late_write = params.find<bool>("late_write", "false");

    pub = params.find<bool>("publish", "true");

    bool double_initialize = params.find<bool>("double_initialize", "false");

    late_initialize = params.find<bool>("late_initialize", "false");

    // Get the verify mode
    std::string mode = params.find<std::string>("verify_mode", "INIT");

    SharedObject::verify_type v_type = SharedObject::INIT_VERIFY;
    if ( mode == "FE" )
        v_type = SharedObject::FE_VERIFY;
    else if ( mode == "NONE" ) {
        v_type = SharedObject::NO_VERIFY;
        // Don't check values, since there is no guarantee what
        // they'll be
        check  = false;
    }

    // Initialize SharedObject
    if ( test_array && !late_initialize ) {
        if ( full_initialization ) {
            if ( myid == 0 || (multiple_initializers && (myid == num_entities - 1)) ) {
                array.initialize("test_shared_array", num_entities, -1, v_type);
            }
            else {
                array.initialize("test_shared_array");
            }
            if ( double_initialize ) array.initialize("test_shared_array", num_entities, -1, v_type);

            if ( myid == 0 || (multiple_initializers && (myid == num_entities - 1)) ) {
                for ( int i = 0; i < num_entities; ++i ) {
                    array.write(i, i + (conflicting_write ? myid : 0));
                }
            }
        }
        else {
            array.initialize("test_shared_array", myid + 1, -1, v_type);
            if ( double_initialize ) array.initialize("test_shared_array", myid + 1, -1, v_type);
            array.write(myid, myid);
        }
        if ( pub ) array.publish();
    }
    else if ( test_map && !late_initialize ) {
        if ( full_initialization ) {
            map.initialize("test_shared_map", v_type);
            if ( double_initialize ) map.initialize("test_shared_map", v_type);
            if ( myid == 0 || (multiple_initializers && (myid == num_entities - 1)) ) {
                for ( int i = 0; i < num_entities; ++i ) {
                    map.write(i, i + (conflicting_write ? myid : 0));
                }
            }
        }
        else {
            map.initialize("test_shared_map", v_type);
            if ( double_initialize ) map.initialize("test_shared_map", v_type);
            map.write(myid, myid);
        }
        if ( pub ) map.publish();
    }
    else if ( test_set && !late_initialize ) {
        if ( full_initialization ) {
            set.initialize("test_shared_set", v_type);
            if ( double_initialize ) set.initialize("test_shared_set", v_type);
            if ( myid == 0 || (multiple_initializers && (myid == num_entities - 1)) ) {
                for ( int i = 0; i < num_entities; ++i ) {
                    set.insert(setItem(i, i + (conflicting_write ? myid : 0)));
                }
            }
        }
        else {
            set.initialize("test_shared_set", v_type);
            if ( double_initialize ) map.initialize("test_shared_set", v_type);
            set.insert(setItem(myid, myid));
        }
        if ( pub ) set.publish();
    }

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    registerClock(
        "1GHz", new Clock::Handler<coreTestSharedObjectsComponent>(this, &coreTestSharedObjectsComponent::tick));
}

void
coreTestSharedObjectsComponent::init(unsigned int UNUSED(phase))
{
    if ( late_initialize ) return;
    // During init, we'll check to see if things are fully published
    if ( test_array ) {
        if ( !array.isFullyPublished() && pub ) {
            out.fatal(CALL_INFO, 100, "ERROR: SharedArray not fully published, but should have been\n");
        }
        if ( array.isFullyPublished() && !pub ) {
            out.fatal(CALL_INFO, 100, "ERROR: SharedArray fully published, but should not have been\n");
        }
    }
    else if ( test_map ) {
        if ( !map.isFullyPublished() && pub ) {
            out.fatal(CALL_INFO, 100, "ERROR: SharedMap not fully published, but should have been\n");
        }
        if ( map.isFullyPublished() && !pub ) {
            out.fatal(CALL_INFO, 100, "ERROR: SharedMap fully published, but should not have been\n");
        }
    }
    else if ( test_set ) {
        if ( !set.isFullyPublished() && pub ) {
            out.fatal(CALL_INFO, 100, "ERROR: SharedSet not fully published, but should have been\n");
        }
        if ( set.isFullyPublished() && !pub ) {
            out.fatal(CALL_INFO, 100, "ERROR: SharedSet fully published, but should not have been\n");
        }
    }
}

void
coreTestSharedObjectsComponent::setup()
{
    if ( late_initialize ) { array.initialize("this_should_fail"); }

    // In setup, we will test that foreach compile for the shared
    // objects and check for late writes
    if ( test_array ) {
        if ( late_write ) { array.write(0, 10); }
        else {
            for ( auto x : array ) {
                if ( x < 0 ) { out.fatal(CALL_INFO, 100, "ERROR: SharedArray data is messed up\n"); }
            }
        }
    }
    else if ( test_map ) {
        if ( late_write ) { map.write(0, 10); }
        else {
            for ( auto x : map ) {
                if ( x.second < 0 ) { out.fatal(CALL_INFO, 100, "ERROR: SharedArray data is messed up\n"); }
            }
        }
    }
    else if ( test_set ) {
        if ( late_write ) { set.insert(setItem(0, 0)); }
        else {
            for ( auto x : set ) {
                if ( x.key < 0 ) { out.fatal(CALL_INFO, 100, "ERROR: SharedSet data is messed up\n"); }
            }
        }
    }
}

void
coreTestSharedObjectsComponent::complete(unsigned int UNUSED(phase))
{}

void
coreTestSharedObjectsComponent::finish()
{}

bool coreTestSharedObjectsComponent::tick(SST::Cycle_t)
{

    if ( check ) {
        // Just check one entry per clock tick
        if ( test_array ) {
            if ( array[count] != count ) { out.fatal(CALL_INFO, 101, "SharedArray does not have the correct data\n"); }
        }
        else if ( test_map ) {
            if ( map[count] != count ) { out.fatal(CALL_INFO, 101, "SharedMap does not have the correct data\n"); }
        }
    }

    count++;

    if ( count == num_entities ) {
        primaryComponentOKToEndSim();
        return true;
    }
    return false;
}

} // namespace CoreTestSharedObjectsComponent
} // namespace SST

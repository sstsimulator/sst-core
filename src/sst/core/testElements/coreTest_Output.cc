// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

//#include <assert.h>

#include "sst_config.h"

#include "sst/core/testElements/coreTest_Output.h"

namespace SST::CoreTestSerialization {

void
testTraceFunction(int level = 0)
{
    TraceFunction trace(CALL_INFO_LONG);
    trace.output("level = %d\n", level);

    if ( level == 0 ) {
        // for first level, output a string long enough that it goes
        // into the overflow case for the string generation in
        // TraceFunction::output().  The current overflow length is 200.
        int         chars_in_line = 0;
        std::string str;
        for ( int i = 0; i < 250; ++i ) {
            str += std::to_string(i % 10);
            chars_in_line++;
            if ( chars_in_line == 40 ) {
                chars_in_line = 0;
                str += '\n';
            }
        }
        trace.output("%s\n", str.c_str());
        testTraceFunction(level + 1);
    }
    else if ( level == 1 || level == 2 ) {
        testTraceFunction(level + 1);
    }
}

coreTestOutput::coreTestOutput(ComponentId_t id, Params& params) : Component(id)
{
    Output& out = getSimulationOutput();

    std::string test = params.find<std::string>("test");
    if ( test == "" ) out.fatal(CALL_INFO_LONG, 1, "ERROR: Must specify test type\n");

    if ( test == "TraceFunction" ) { testTraceFunction(); }
}

} // namespace SST::CoreTestSerialization

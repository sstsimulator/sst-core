// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

//#include <assert.h>

#include "sst_config.h"

#include "sst/core/testElements/coreTest_ParamComponent.h"

#include <cinttypes>

namespace SST {
namespace CoreTestParamComponent {

coreTestParamComponent::coreTestParamComponent(ComponentId_t id, Params& params) : Component(id)
{
    Output& out = getSimulationOutput();
    out.output("Component %s:\n", getName().c_str());
    if ( !params.contains("set_param") ) {
        // Sets in python are not ordered so we can't compare the raw
        // string in the set parameters
        out.output("  Contents of Params object (%" PRIu64 "):\n", id);
        params.print_all_params(out, "    ");
    }

    out.output("  Results of find() calls:\n");

    const std::string i32v_str = params.find<std::string>("int32t_param");
    const int32_t     i32v     = params.find<int32_t>("int32t_param");
    out.output("    int32_t      value = \"%s\" = %" PRId32 "\n", i32v_str.c_str(), i32v);

    const std::string u32v_str = params.find<std::string>("uint32t_param");
    const uint32_t    u32v     = params.find<uint32_t>("uint32t_param");
    out.output("    uint32_t     value = \"%s\" = %" PRIu32 "\n", u32v_str.c_str(), u32v);

    const std::string i64v_str = params.find<std::string>("int64t_param");
    const int64_t     i64v     = params.find<int64_t>("int64t_param");
    out.output("    int64_t      value = \"%s\" = %" PRId64 "\n", i64v_str.c_str(), i64v);

    const std::string u64v_str = params.find<std::string>("uint64t_param");
    const uint64_t    u64v     = params.find<uint64_t>("uint64t_param");
    out.output("    uint64_t     value = \"%s\" = %" PRIu64 "\n", u64v_str.c_str(), u64v);

    const std::string btruev_str = params.find<std::string>("bool_true_param");
    const bool        btruev     = params.find<bool>("bool_true_param");
    out.output("    bool_true    value = \"%s\" = %s\n", btruev_str.c_str(), (btruev ? "true" : "false"));

    const std::string bfalsev_str = params.find<std::string>("bool_false_param");
    const bool        bfalsev     = params.find<bool>("bool_false_param");
    out.output("    bool_false   value = \"%s\" = %s\n", bfalsev_str.c_str(), (bfalsev ? "true" : "false"));

    const std::string f32v_str = params.find<std::string>("float_param");
    const float       f32v     = params.find<float>("float_param");
    out.output("    float        value = \"%s\" = %f = %e\n", f32v_str.c_str(), f32v, f32v);

    const std::string f64v_str = params.find<std::string>("double_param");
    const double      f64v     = params.find<double>("double_param");
    out.output("    double       value = \"%s\" = %lf = %e\n", f64v_str.c_str(), f64v, f64v);

    const std::string strv = params.find<std::string>("string_param");
    out.output("    string       value = \"%s\"\n", strv.c_str());

    // Test scoped params
    Params p = params.get_scoped_params("scope");
    out.output("    Scoped Params:\n");
    p.print_all_params(out, "      ");

    // Test array params
    std::vector<int> array;
    params.find_array("array_param", array);
    out.output("    array = [");
    bool first = true;
    for ( auto& i : array ) {
        if ( first ) {
            out.output(" %d", i);
            first = false;
        }
        else
            out.output(", %d", i);
    }
    out.output(" ]\n");

    // Test set params
    std::set<std::string> set;
    params.find_set<std::string>("set_param", set);
    out.output("    set = {");
    first = true;
    for ( auto& i : set ) {
        if ( first ) {
            out.output(" %s", i.c_str());
            first = false;
        }
        else
            out.output(", %s", i.c_str());
    }
    out.output(" }\n");

    // Test map params
    std::map<std::string, int> map;
    params.find_map<std::string, int>("map_param", map);
    out.output("    map = {");
    first = true;
    for ( auto& i : map ) {
        if ( first ) {
            out.output(" %s : %d", i.first.c_str(), i.second);
            first = false;
        }
        else
            out.output(", %s: %d", i.first.c_str(), i.second);
    }
    out.output(" }\n\n");
}


coreTestParamComponent::coreTestParamComponent() : Component(-1) {}

} // namespace CoreTestParamComponent
} // namespace SST

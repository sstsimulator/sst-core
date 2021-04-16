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

//#include <assert.h>

#include "sst_config.h"

#include "sst/core/testElements/coreTest_ParamComponent.h"

namespace SST {
namespace CoreTestParamComponent {

coreTestParamComponent::coreTestParamComponent(ComponentId_t id, Params& params) :
  Component(id)
{
	const std::string i32v_str = params.find<std::string>("int32t-param");
	const int32_t i32v = params.find<int32_t>("int32t-param");
	printf("int32_t      value = \"%s\" = %" PRId32 "\n", i32v_str.c_str(), i32v);

	const std::string u32v_str = params.find<std::string>("uint32t-param");
	const uint32_t u32v = params.find<uint32_t>("uint32t-param");
	printf("uint32_t     value = \"%s\" = %" PRIu32 "\n", u32v_str.c_str(), u32v);

	const std::string i64v_str = params.find<std::string>("int64t-param");
	const int64_t i64v = params.find<int64_t>("int64t-param");
	printf("int64_t      value = \"%s\" = %" PRId64 "\n", i64v_str.c_str(), i64v);

	const std::string u64v_str = params.find<std::string>("uint64t-param");
	const uint64_t u64v = params.find<uint64_t>("uint64t-param");
	printf("uint64_t     value = \"%s\" = %" PRIu64 "\n", u64v_str.c_str(), u64v);

	const std::string btruev_str = params.find<std::string>("bool-true-param");
	const bool btruev = params.find<bool>("bool-true-param");
	printf("bool-true    value = \"%s\" = %s\n", btruev_str.c_str(), (btruev ? "true" : "false"));

	const std::string bfalsev_str = params.find<std::string>("bool-false-param");
	const bool bfalsev = params.find<bool>("bool-false-param");
	printf("bool-false   value = \"%s\" = %s\n", bfalsev_str.c_str(), (bfalsev ? "true" : "false"));

	const std::string f32v_str = params.find<std::string>("float-param");
	const float f32v = params.find<float>("float-param");
	printf("float        value = \"%s\" = %f = %e\n", f32v_str.c_str(), f32v, f32v);

	const std::string f64v_str = params.find<std::string>("double-param");
	const double f64v = params.find<double>("double-param");
	printf("double       value = \"%s\" = %lf = %e\n", f64v_str.c_str(), f64v, f64v);

	const std::string strv = params.find<std::string>("string-param");
	printf("string       value = \"%s\"\n", strv.c_str());


    // Test scoped params
    Params p = params.get_scoped_params("scope");
    p.print_all_params(Simulation::getSimulationOutput());
}

coreTestParamComponent::coreTestParamComponent() :
    Component(-1)
{

}

} // namespace coreTestParamComponent
} // namespace SST



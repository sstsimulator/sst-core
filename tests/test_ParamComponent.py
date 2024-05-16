# Copyright 2009-2024 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2024, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
import sst

# Define SST core options
sst.setProgramOption("stop-at", "25us")

global_params = {
    "bool_true_param" : "yes",
    "bool_false_param" : "no",
    "scope.string" : "scope test"
    }

sst.addGlobalParams("test_set",global_params)

# Define the simulation components
param_c0 = sst.Component("c0", "coreTestElement.coreTestParamComponent")
param_c0.addParams({
	"int32t_param" : 2147483647,
	"uint32t_param" : "4294967295",
	"int64t_param" : 9223372036854775807,
	"uint64t_param" : 18446744073709551615,
	"bool_true_param" : True,
	"bool_false_param" : False,
	"float_param" : 1.0101,
	"double_param" : 1.3333e-10
})
param_c0.addGlobalParamSet("test_set")
param_c0.addGlobalParamSet("test_set2")

# Define the simulation components
param_c1 = sst.Component("c1", "coreTestElement.coreTestParamComponent")
param_c1.addParams({
	"int32t_param" : -2147483648,
	"uint32t_param" : 0,
	"int64t_param" : -9223372036854775808,
	"uint64t_param" : 18446744073709551615,
	"bool_true_param" : "true",
	"bool_false_param" : "false",
	"float_param" : 1.00000000e-15,
	"double_param" : 1.3333e-52
})
param_c1.addGlobalParamSet("test_set")
param_c1.addGlobalParamSet("test_set2")

# Define the simulation components
param_c2 = sst.Component("c2", "coreTestElement.coreTestParamComponent")
param_c2.addParams({
	"int32t_param" : "2147483647",
	"uint32t_param" : "4294967295",
	"int64t_param" : "-9223372036854775808",
	"uint64t_param" : "18446744073709551615",
	"bool_true_param" : 1,
	"bool_false_param" : 0,
	"float_param" : "-1.0000e-15",
	"double_param" : "-1.33e7",
	"string_param" : "teststring456"
})
param_c2.addGlobalParamSet("test_set")
param_c2.addGlobalParamSet("test_set2")

# Define the simulation components
param_c3 = sst.Component("c3", "coreTestElement.coreTestParamComponent")
param_c3.addParams({
	"int32t_param" : "2147483647",
	"uint32t_param" : "4294967295",
	"int64t_param" : "-9223372036854775808",
	"uint64t_param" : "18446744073709551615",
	"float_param" : "-1.0000e-15",
	"double_param" : "-1.33e7",
	"string_param" : "teststring456",
        "scope.int32" : "100",
        "scope.bool" : "no"
})
param_c3.addGlobalParamSet("test_set")
param_c3.addGlobalParamSet("test_set2")

# Define the simulation components
array_param = [ 1, 2, "4", "8" ]
set_param = { "one", "two", 3, True }
map_param = { "one" : "1", "two" : 2, "three" : 3, "four" : 4 }
param_c4 = sst.Component("c4", "coreTestElement.coreTestParamComponent")
param_c4.addParams({
        "array_param" : array_param,
        "set_param" : set_param,
        "map_param" : map_param
})

sst.addGlobalParam("test_set2","string_param","teststring123")

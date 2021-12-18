# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "25us")

global_params = {
    "bool-true-param" : "yes",
    "bool-false-param" : "no",
    "scope.string" : "scope test"
    }

sst.addGlobalParam("test_set","string-param","teststring123")
sst.addGlobalParams("test_set",global_params)

# Define the simulation components
param_c0 = sst.Component("c0.0", "coreTestElement.coreTestParamComponent")
param_c0.addParams({
	"int32t-param" : 2147483647,
	"uint32t-param" : "4294967295",
	"int64t-param" : 9223372036854775807,
	"uint64t-param" : 18446744073709551615,
	"bool-true-param" : True,
	"bool-false-param" : False,
	"float-param" : 1.0101,
	"double-param" : 1.3333e-10
})
param_c0.addGlobalParamSet("test_set")

# Define the simulation components
param_c1 = sst.Component("c1.0", "coreTestElement.coreTestParamComponent")
param_c1.addParams({
	"int32t-param" : -2147483648,
	"uint32t-param" : 0,
	"int64t-param" : -9223372036854775808,
	"uint64t-param" : 18446744073709551615,
	"bool-true-param" : "true",
	"bool-false-param" : "false",
	"float-param" : 1.00000000e-15,
	"double-param" : 1.3333e-52
})
param_c1.addGlobalParamSet("test_set")

# Define the simulation components
param_c2 = sst.Component("c2.0", "coreTestElement.coreTestParamComponent")
param_c2.addParams({
	"int32t-param" : "2147483647",
	"uint32t-param" : "4294967295",
	"int64t-param" : "-9223372036854775808",
	"uint64t-param" : "18446744073709551615",
	"bool-true-param" : 1,
	"bool-false-param" : 0,
	"float-param" : "-1.0000e-15",
	"double-param" : "-1.33e7",
	"string-param" : "teststring456"
})
param_c2.addGlobalParamSet("test_set")

# Define the simulation components
param_c3 = sst.Component("c3.0", "coreTestElement.coreTestParamComponent")
param_c3.addParams({
	"int32t-param" : "2147483647",
	"uint32t-param" : "4294967295",
	"int64t-param" : "-9223372036854775808",
	"uint64t-param" : "18446744073709551615",
	"float-param" : "-1.0000e-15",
	"double-param" : "-1.33e7",
	"string-param" : "teststring456",
        "scope.int32" : "100",
        "scope.bool" : "no"
})
param_c3.addGlobalParamSet("test_set")

# Define the simulation components
array_param = [ 1, 2, "4", "8" ]
set_param = { "one", "two", 3, True }
map_param = { "one" : "1", "two" : 2, "three" : 3, "four" : 4 }
param_c4 = sst.Component("c4.0", "coreTestElement.coreTestParamComponent")
param_c4.addParams({
        "array_param" : array_param,
        "set_param" : set_param,
        "map_param" : map_param
})

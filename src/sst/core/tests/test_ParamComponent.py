# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "25us")

# Define the simulation components
param_c0 = sst.Component("c0.0", "simpleElementExample.simpleParamComponent")
param_c0.addParams({
	"int32t-param" : 2147483647,
	"uint32t-param" : "4294967295",
	"int64t-param" : 9223372036854775807,
	"uint64t-param" : 18446744073709551615,
	"bool-true-param" : True,
	"bool-false-param" : False,
	"float-param" : 1.0101,
	"double-param" : 1.3333e-10,
	"string-param" : "teststring123"
})

# Define the simulation components
param_c1 = sst.Component("c1.0", "simpleElementExample.simpleParamComponent")
param_c1.addParams({
	"int32t-param" : -2147483648,
	"uint32t-param" : 0,
	"int64t-param" : -9223372036854775808,
	"uint64t-param" : 18446744073709551615,
	"bool-true-param" : "true",
	"bool-false-param" : "false",
	"float-param" : 1.00000000e-15,
	"double-param" : 1.3333e-52,
	"string-param" : "teststring123"
})

# Define the simulation components
param_c2 = sst.Component("c2.0", "simpleElementExample.simpleParamComponent")
param_c2.addParams({
	"int32t-param" : "2147483647",
	"uint32t-param" : "4294967295",
	"int64t-param" : "-9223372036854775808",
	"uint64t-param" : "18446744073709551615",
	"bool-true-param" : 1,
	"bool-false-param" : 0,
	"float-param" : "-1.0000e-15",
	"double-param" : "-1.33e-18",
	"string-param" : "teststring123"
})

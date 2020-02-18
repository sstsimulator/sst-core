import sst

d0 = sst.Component("d0", "simpleElementExample.simpleDistribComponent")
d0.addParams({
		"distrib" : "poisson",
		"lambda" : "8",
		"mean" : "32",
		"stddev" : "4",
		"count" : "100000000",
		"binresults" : "1"
        })

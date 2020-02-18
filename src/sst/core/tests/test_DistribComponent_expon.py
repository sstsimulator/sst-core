import sst

d0 = sst.Component("d0", "simpleElementExample.simpleDistribComponent")
d0.addParams({
		"distrib" : "exponential",
		"lambda" : "0.5",
		"count" : "100000000",
		"binresults" : "1"
        })

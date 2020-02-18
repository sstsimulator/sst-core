import sst

d0 = sst.Component("d0", "simpleElementExample.simpleDistribComponent")
d0.addParams({
		"distrib" : "discrete",
		"probcount" : "5",
		"prob0" : "0.1",
		"prob1" : "0.3",
		"prob2" : "0.35",
		"prob3" : "0.15",
		"prob4" : "0.1",
		"count" : "100000000",
		"binresults" : "1"
        })

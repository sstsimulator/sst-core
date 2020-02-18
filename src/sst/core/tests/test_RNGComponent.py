import sst

def sstcreatemodel():

    sst.creategraph()

    if sst.verbose():
	print "SST Random Number Generation Test Component Model"

    id = sst.createcomponent("rng0", "simpleElementExample.simpleRNGComponent")
    sst.addcompparam(id, "count", "1000")
    sst.addcompparam(id, "seed_w", "1447")
    sst.addcompparam(id, "seed_z", "1053")

    return 0

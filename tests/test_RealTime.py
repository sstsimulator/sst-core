import sst
import sys

# Note: test WILL NOT terminate on its own
if len(sys.argv) == 2 and sys.argv[1] == "stop":
    sst.setProgramOption("stop-at", "25us")

# Define the simulation components
comp_map = []
for x in range(10):
    comp_map.append([])
    for y in range(10):
        comp = sst.Component("c" + str(x) + "_" + str(y), "coreTestElement.coreTestComponent")
        comp.addParams({
            "workPerCycle" : "1000",
            "commSize" : "100",
            "commFreq" : "1000"
        })
        comp_map[x].append(comp)

# Define the simulation links
## E-W
for x in range(10):
    for y in range(10):
        west_x = (x + 1) % 10
        link_EW = sst.Link("link_EW_" + str(x) + "_" + str(y))
        link_EW.connect( (comp_map[x][y], "Elink", "10000ps"), (comp_map[west_x][y], "Wlink", "10000ps") )
        north_y = (y + 1) % 10
        link_NS = sst.Link("link_NS_" + str(x) + "_" + str(y))
        link_NS.connect( (comp_map[x][y], "Slink", "10000ps"), (comp_map[x][north_y], "Nlink", "10000ps") )


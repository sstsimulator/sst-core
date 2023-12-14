import sst

# Define SST core options
#sst.setProgramOption("stop-at", "25us")

# Define the simulation components
comp_c0_0 = sst.Component("c0_0", "coreTestElement.coreTestComponent")
comp_c0_0.addParams({
    "workPerCycle" : "100",
    "commSize" : "100",
    "commFreq" : "1000",
    "clockFrequency" : "60s"
})

comp_c0_1 = sst.Component("c0_1", "coreTestElement.coreTestComponent")
comp_c0_1.addParams({
    "workPerCycle" : "100",
    "commSize" : "100",
    "commFreq" : "1000",
    "clockFrequency" : "60s"
})

comp_c1_0 = sst.Component("c1_0", "coreTestElement.coreTestComponent")
comp_c1_0.addParams({
    "workPerCycle" : "100",
    "commSize" : "100",
    "commFreq" : "1000",
    "clockFrequency" : "60s"
})

comp_c1_1 = sst.Component("c1_1", "coreTestElement.coreTestComponent")
comp_c1_1.addParams({
    "workPerCycle" : "100",
    "commSize" : "100",
    "commFreq" : "1000",
    "clockFrequency" : "60s"
})

# Define the simulation links

# North/South links
link_ns_0_01 = sst.Link("link_ns_0_01")
link_ns_0_01.connect( (comp_c0_0, "Nlink", "10000ps"), (comp_c0_1, "Slink", "10000ps") )

link_ns_0_10 = sst.Link("link_ns_0_10")
link_ns_0_10.connect( (comp_c0_0, "Slink", "10000ps"), (comp_c0_1, "Nlink", "10000ps") )

link_ns_1_01 = sst.Link("link_ns_1_01")
link_ns_1_01.connect( (comp_c1_0, "Nlink", "10000ps"), (comp_c1_1, "Slink", "10000ps") )

link_ns_0_10 = sst.Link("link_ns_1_10")
link_ns_0_10.connect( (comp_c1_0, "Slink", "10000ps"), (comp_c1_1, "Nlink", "10000ps") )

# East/West links
link_ew_0_01 = sst.Link("link_ew_0_01")
link_ew_0_01.connect( (comp_c0_0, "Elink", "10000ps"), (comp_c1_0, "Wlink", "10000ps") )

link_ew_0_10 = sst.Link("link_ew_0_10")
link_ew_0_10.connect( (comp_c0_0, "Wlink", "10000ps"), (comp_c1_0, "Elink", "10000ps") )

link_ew_1_01 = sst.Link("link_ew_1_01")
link_ew_1_01.connect( (comp_c0_1, "Elink", "10000ps"), (comp_c1_1, "Wlink", "10000ps") )

link_ew_1_10 = sst.Link("link_ew_1_10")
link_ew_1_10.connect( (comp_c0_1, "Wlink", "10000ps"), (comp_c1_1, "Elink", "10000ps") )

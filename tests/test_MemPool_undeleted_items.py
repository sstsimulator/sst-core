import sst

# Define SST core options
sst.setProgramOption("stopAtCycle", "10000ns")
sst.setProgramOption("output-undeleted-events", "STDOUT")

# Define the simulation components
comp0 = sst.Component("c0", "coreTestElement.memPoolTestComponent")
comp0.addParams({
    "event_size" : 1,
    "undeleted_events" : 3,
    "check_overflow" : False,
})

comp1 = sst.Component("c1", "coreTestElement.memPoolTestComponent")
comp1.addParams({
    "event_size" : 2,
    "undeleted_events" : 3,
    "check_overflow" : False,
})

comp2 = sst.Component("c2", "coreTestElement.memPoolTestComponent")
comp2.addParams({
    "event_size" : 3,
    "undeleted_events" : 3,
    "check_overflow" : False,
})

comp3 = sst.Component("c3", "coreTestElement.memPoolTestComponent")
comp3.addParams({
    "event_size" : 4,
    "undeleted_events" : 3,
    "check_overflow" : False,
})


# Define the simulation links
link_c0_c1 = sst.Link("link_c0_c1")
link_c0_c1.connect( (comp0, "port0", "1ns"), (comp1, "port0", "1ns") )

link_c0_c2 = sst.Link("link_c0_c2")
link_c0_c2.connect( (comp0, "port1", "1ns"), (comp2, "port0", "1ns") )

link_c0_c3 = sst.Link("link_c0_c3")
link_c0_c3.connect( (comp0, "port2", "1ns"), (comp3, "port0", "1ns") )


link_c1_c2 = sst.Link("link_c1_c2")
link_c1_c2.connect( (comp1, "port1", "1ns"), (comp2, "port1", "1ns") )

link_c1_c3 = sst.Link("link_c1_c3")
link_c1_c3.connect( (comp1, "port2", "1ns"), (comp3, "port1", "1ns") )


link_c2_c3 = sst.Link("link_c2_c3")
link_c2_c3.connect( (comp2, "port2", "1ns"), (comp3, "port2", "1ns") )


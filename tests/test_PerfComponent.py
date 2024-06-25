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
sst.setProgramOption("stop-at", "10us")

# Define the simulation components
comp_c0_0 = sst.Component("c0_0", "coreTestElement.coreTestPerfComponent")
comp_c0_0.addParams({
      "workPerCycle" : "5000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c1_0 = sst.Component("c1_0", "coreTestElement.coreTestPerfComponent")
comp_c1_0.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c2_0 = sst.Component("c2_0", "coreTestElement.coreTestPerfComponent")
comp_c2_0.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c3_0 = sst.Component("c3_0", "coreTestElement.coreTestPerfComponent")
comp_c3_0.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c0_1 = sst.Component("c0_1", "coreTestElement.coreTestPerfComponent")
comp_c0_1.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c1_1 = sst.Component("c1_1", "coreTestElement.coreTestPerfComponent")
comp_c1_1.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c2_1 = sst.Component("c2_1", "coreTestElement.coreTestPerfComponent")
comp_c2_1.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c3_1 = sst.Component("c3_1", "coreTestElement.coreTestPerfComponent")
comp_c3_1.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c0_2 = sst.Component("c0_2", "coreTestElement.coreTestPerfComponent")
comp_c0_2.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c1_2 = sst.Component("c1_2", "coreTestElement.coreTestPerfComponent")
comp_c1_2.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c2_2 = sst.Component("c2_2", "coreTestElement.coreTestPerfComponent")
comp_c2_2.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c3_2 = sst.Component("c3_2", "coreTestElement.coreTestPerfComponent")
comp_c3_2.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c0_3 = sst.Component("c0_3", "coreTestElement.coreTestPerfComponent")
comp_c0_3.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c1_3 = sst.Component("c1_3", "coreTestElement.coreTestPerfComponent")
comp_c1_3.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c2_3 = sst.Component("c2_3", "coreTestElement.coreTestPerfComponent")
comp_c2_3.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})
comp_c3_3 = sst.Component("c3_3", "coreTestElement.coreTestPerfComponent")
comp_c3_3.addParams({
      "workPerCycle" : "1000",
      "commSize" : "100",
      "commFreq" : "1000"
})

# Define the simulation links
link_s_0_10 = sst.Link("link_s_0_10")
link_s_0_10.connect( (comp_c0_0, "Nlink", "10000ps"), (comp_c0_1, "Slink", "10000ps") )
link_s_0_90 = sst.Link("link_s_0_90")
link_s_0_90.connect( (comp_c0_0, "Slink", "10000ps"), (comp_c0_3, "Nlink", "10000ps") )
link_s_0_1 = sst.Link("link_s_0_1")
link_s_0_1.connect( (comp_c0_0, "Elink", "10000ps"), (comp_c1_0, "Wlink", "10000ps") )
link_s_0_9 = sst.Link("link_s_0_9")
link_s_0_9.connect( (comp_c0_0, "Wlink", "10000ps"), (comp_c3_0, "Elink", "10000ps") )
link_s_1_11 = sst.Link("link_s_1_11")
link_s_1_11.connect( (comp_c1_0, "Nlink", "10000ps"), (comp_c1_1, "Slink", "10000ps") )
link_s_1_91 = sst.Link("link_s_1_91")
link_s_1_91.connect( (comp_c1_0, "Slink", "10000ps"), (comp_c1_3, "Nlink", "10000ps") )
link_s_1_2 = sst.Link("link_s_1_2")
link_s_1_2.connect( (comp_c1_0, "Elink", "10000ps"), (comp_c2_0, "Wlink", "10000ps") )
link_s_2_12 = sst.Link("link_s_2_12")
link_s_2_12.connect( (comp_c2_0, "Nlink", "10000ps"), (comp_c2_1, "Slink", "10000ps") )
link_s_2_92 = sst.Link("link_s_2_92")
link_s_2_92.connect( (comp_c2_0, "Slink", "10000ps"), (comp_c2_3, "Nlink", "10000ps") )
link_s_2_3 = sst.Link("link_s_2_3")
link_s_2_3.connect( (comp_c2_0, "Elink", "10000ps"), (comp_c3_0, "Wlink", "10000ps") )
link_s_3_13 = sst.Link("link_s_3_13")
link_s_3_13.connect( (comp_c3_0, "Nlink", "10000ps"), (comp_c3_1, "Slink", "10000ps") )
link_s_3_93 = sst.Link("link_s_3_93")
link_s_3_93.connect( (comp_c3_0, "Slink", "10000ps"), (comp_c3_3, "Nlink", "10000ps") )
link_s_10_20 = sst.Link("link_s_10_20")
link_s_10_20.connect( (comp_c0_1, "Nlink", "10000ps"), (comp_c0_2, "Slink", "10000ps") )
link_s_10_11 = sst.Link("link_s_10_11")
link_s_10_11.connect( (comp_c0_1, "Elink", "10000ps"), (comp_c1_1, "Wlink", "10000ps") )
link_s_10_19 = sst.Link("link_s_10_19")
link_s_10_19.connect( (comp_c0_1, "Wlink", "10000ps"), (comp_c3_1, "Elink", "10000ps") )
link_s_11_21 = sst.Link("link_s_11_21")
link_s_11_21.connect( (comp_c1_1, "Nlink", "10000ps"), (comp_c1_2, "Slink", "10000ps") )
link_s_11_12 = sst.Link("link_s_11_12")
link_s_11_12.connect( (comp_c1_1, "Elink", "10000ps"), (comp_c2_1, "Wlink", "10000ps") )
link_s_12_22 = sst.Link("link_s_12_22")
link_s_12_22.connect( (comp_c2_1, "Nlink", "10000ps"), (comp_c2_2, "Slink", "10000ps") )
link_s_12_13 = sst.Link("link_s_12_13")
link_s_12_13.connect( (comp_c2_1, "Elink", "10000ps"), (comp_c3_1, "Wlink", "10000ps") )
link_s_13_23 = sst.Link("link_s_13_23")
link_s_13_23.connect( (comp_c3_1, "Nlink", "10000ps"), (comp_c3_2, "Slink", "10000ps") )
link_s_20_30 = sst.Link("link_s_20_30")
link_s_20_30.connect( (comp_c0_2, "Nlink", "10000ps"), (comp_c0_3, "Slink", "10000ps") )
link_s_20_21 = sst.Link("link_s_20_21")
link_s_20_21.connect( (comp_c0_2, "Elink", "10000ps"), (comp_c1_2, "Wlink", "10000ps") )
link_s_20_29 = sst.Link("link_s_20_29")
link_s_20_29.connect( (comp_c0_2, "Wlink", "10000ps"), (comp_c3_2, "Elink", "10000ps") )
link_s_21_31 = sst.Link("link_s_21_31")
link_s_21_31.connect( (comp_c1_2, "Nlink", "10000ps"), (comp_c1_3, "Slink", "10000ps") )
link_s_21_22 = sst.Link("link_s_21_22")
link_s_21_22.connect( (comp_c1_2, "Elink", "10000ps"), (comp_c2_2, "Wlink", "10000ps") )
link_s_22_32 = sst.Link("link_s_22_32")
link_s_22_32.connect( (comp_c2_2, "Nlink", "10000ps"), (comp_c2_3, "Slink", "10000ps") )
link_s_22_23 = sst.Link("link_s_22_23")
link_s_22_23.connect( (comp_c2_2, "Elink", "10000ps"), (comp_c3_2, "Wlink", "10000ps") )
link_s_23_33 = sst.Link("link_s_23_33")
link_s_23_33.connect( (comp_c3_2, "Nlink", "10000ps"), (comp_c3_3, "Slink", "10000ps") )
link_s_30_31 = sst.Link("link_s_30_31")
link_s_30_31.connect( (comp_c0_3, "Elink", "10000ps"), (comp_c1_3, "Wlink", "10000ps") )
link_s_30_39 = sst.Link("link_s_30_39")
link_s_30_39.connect( (comp_c0_3, "Wlink", "10000ps"), (comp_c3_3, "Elink", "10000ps") )
link_s_31_32 = sst.Link("link_s_31_32")
link_s_31_32.connect( (comp_c1_3, "Elink", "10000ps"), (comp_c2_3, "Wlink", "10000ps") )
link_s_32_33 = sst.Link("link_s_32_33")
link_s_32_33.connect( (comp_c2_3, "Elink", "10000ps"), (comp_c3_3, "Wlink", "10000ps") )

# Copyright 2009-2023 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2023, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
import sst

# Define SST core options
sst.setProgramOption("stop-at", "25us")

# Define the simulation components
size = 2
params = {
  "workPerCycle" : 1000,
  "commSize" : 100,
  "commFreq" : 1000,
}

def make_component(row, col):
  return sst.Component("c%d.%d" % (row,col), "coreTestElement.coreTestComponent")

comps = [[make_component(i,j) for i in range(size)] for j in range(size)]

def shift(size, index, shift):
    return (index + shift + size) % size

def connect(src_port, dst_port, comps, row, col, shift_x=0, shift_y=0, latency="10000ps"):
    size = len(comps)
    nbr_row = (row + shift_x + size) % size
    nbr_col = (col + shift_y + size) % size
    link = sst.Link("%s_%d_%d_%d_%d" % (src_port, row, col, nbr_row, nbr_col))
    src = comps[row][col]
    dst = comps[nbr_row][nbr_col]
    link.connect((src, src_port, latency), (dst, dst_port, latency))
    

for row in range(size):
  for col in range(size):
    connect("Nlink", "Slink", comps, row, col, shift_y=1)
    connect("Elink", "Wlink", comps, row, col, shift_x=1)

    comp = comps[row][col]
    comp.addParams(params)
    counts = comp.createStatistic("counts", {"type" : "sst.HistogramStatistic"})
    comp.setStatistic("N", counts)
    comp.setStatistic("S", counts)
    comp.setStatistic("E", counts)
    comp.setStatistic("W", counts)

sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputCSV")

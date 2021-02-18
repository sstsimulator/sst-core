# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "25us")

# Define the simulation components
comps
size = 2
params = {
  "workPerCycle" : 1000,
  "commSize" : 100,
  "commFreq" : 1000,
}

def make_component(row, col):
  return sst.Component("c%d.%d" % (i,j), "coreTestElement.coreTestComponent")

comps = [[make_component(i,j) for i in range(size)] for j in range(size)]

def shift(size, index, shift):
    return (index + shift + size) % size

def connect(src_port, dst_port, comps, row, col, shift_x=0, shift_y=0, latency="10000ps"):
    size = len(comps)
    nbr_row = (row + shift_x + size) % size
    nbr_col = (col + shift_y + size) % size
    link = sst.Link("link_%d_%d_%d_%d" % (row, col, nbr_row, nbr_col))
    src = comps[row][col]
    dst = comps[nbr_row][nbr_col]
    link.connect((src, src_port, latency), (dst, dst_port, latency))
    

for row in range(size):
  for col in range(size):
    connect("Nlink", "Slink", comps, row, col, shift_y=1)
    connect("Slink", "Nlink", comps, row, col, shift_y=-1)
    connect("Elink", "Wlink", comps, row, col, shift_x=1)
    connect("Wlink", "Elink", comps, row, col, shift_x=-1)

    comp = comps[row][col]
    counts = comp.createStatistic("counts", None) #no subid
    comp.reuseStatistic("N", counts)
    comp.reuseStatistic("S", counts)
    comp.reuseStatistic("E", counts)
    comp.reuseStatistic("W", counts)

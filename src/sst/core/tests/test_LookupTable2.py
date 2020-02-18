import sst
import inspect, os, sys

nitems = 10

params = dict({
    "num_entities" : nitems
    })

for i in range(nitems):
    comp = sst.Component("Table Comp %d"%i, "simpleElementExample.simpleLookupTableComponent")
    comp.addParams(params)
    comp.addParam("myid", i)


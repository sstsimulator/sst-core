import sst
import inspect, os, sys


params = dict({
    # Set filename to the name of this file
    "filename" : inspect.getfile(inspect.currentframe())
    })

for i in range(10):
    comp = sst.Component("Table Comp %d"%i, "simpleElementExample.simpleLookupTableComponent")
    comp.addParams(params)


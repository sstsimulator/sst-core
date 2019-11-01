import sst

nports = 2
a = sst.Component("a", "ctest.ctest")
b = sst.Component("b", "ctest.ctest")

params = dict(num_ports=nports, active_ports=[0,1], num_init_events=[2,1])

a.addParams(params)
b.addParams(params)

for p in range(nports):
  link = sst.Link("a:%d-b:%d" % (p,p))
  link.connect((a,"port%d"%p,"10ns"),(b,"port%d"%p,"10ns"))

sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputCSV")
a.enableStatistics(["observed_numbers"], {"type":"sst.HistogramStatistic"})



import sst

nports = 2
a = sst.Component("a", "coreTestElement.coreTestStatisticIntensityComponent")
b = sst.Component("b", "coreTestElement.coreTestStatisticIntensityComponent")

params = dict(num_ports=nports, num_init_events=[2, 1])

a.addParams(params)
b.addParams(params)

portParams = dict(seed=15)

sideX = 0.5
sideY = 0.5
sideZ = 0.5
for p in range(nports):
    stat_paramsA = dict(
      origin=[0, 0 + p, 0],
      size=[sideX, sideY, sideZ],
      shape="cube",
      type="sst.IntensityStatistic",
    )
    stat_paramsB = dict(
      origin=[0, 2 + p, 0],
      size=[sideX, sideY, sideZ],
      shape="cube",
      type="sst.IntensityStatistic",
    )
    portA = a.setSubComponent("port%d" % p, "coreTestElement.coreTestStatisticIntensityActivePort")
    portB = b.setSubComponent("port%d" % p, "coreTestElement.coreTestStatisticIntensityActivePort")
    portA.addParams(portParams)
    portB.addParams(portParams)
    link = sst.Link("a:%d-b:%d" % (p, p))
    latency = 10 + p * 5
    link.connect((portA, "outport", "%dns" % latency),
                 (portB, "outport", "%dns" % latency))
    portA.enableStatistics(["traffic_intensity"], stat_paramsA)
    portB.enableStatistics(["traffic_intensity"], stat_paramsB)

sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.vtkstatisticoutputexodus")

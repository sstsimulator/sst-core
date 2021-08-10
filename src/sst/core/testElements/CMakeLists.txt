add_library(
  coreTestElement MODULE
  coreTest_Component.cc
  coreTest_ClockerComponent.cc
  coreTest_DistribComponent.cc
  coreTest_RNGComponent.cc
  coreTest_StatisticsComponent.cc
  coreTest_Links.cc
  coreTest_MessageGeneratorComponent.cc
  coreTest_SharedObjectComponent.cc
  coreTest_SubComponent.cc
  coreTest_Module.cc
  coreTest_ParamComponent.cc)

# NEED To double check this
target_compile_definitions(coreTestElement
                           PUBLIC "LINKER:-module,-avoid-version")

install(TARGETS coreTestElement DESTINATION lib/sstcore)

# NEED To figure out how to deal with distributing the README

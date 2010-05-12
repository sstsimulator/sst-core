# -*- Makefile -*-
#
#

genTopo_SOURCES = \
	src_genTopo/dot.c \
	src_genTopo/gen.c \
	src_genTopo/main.c \
	src_genTopo/sst_gen.c \
	src_genTopo/topo_flat2Dbutter.c \
	src_genTopo/topo_full.c \
	src_genTopo/topo_hyper.c \
	src_genTopo/topo_mesh2D.c \
	src_genTopo/topo_mesh3D.c \
	src_genTopo/topo_ring.c \
	src_genTopo/topo_tree.c



bin_PROGRAMS = genTopo

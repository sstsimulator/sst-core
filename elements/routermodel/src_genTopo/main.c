/*
** $Id: main.c,v 1.20 2010/04/30 20:26:19 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include "main.h"
#include "dot.h"
#include "sst_gen.h"
#include "topo_mesh2D.h"
#include "topo_mesh3D.h"
#include "topo_flat2Dbutter.h"
#include "topo_ring.h"
#include "topo_full.h"
#include "topo_tree.h"
#include "topo_hyper.h"

#define MAX_ID_LEN		(256)



/*
** Local functions
*/
static void usage(char *argv[]);
static int is_pow2(int num);
static int pow2(int dim);




/*
** Long options
*/
static struct option long_options[]=   {
    /* name, has arg, flag, val */
    {"help", 0, NULL, 9999},
    {"topology", 1, NULL, 't'},
    {"dotfilename", 1, NULL, 'd'},
    {"sstfilename", 1, NULL, 's'},
    {"nodes", 1, NULL, 'n'},
    {"cpu_verbose", 1, NULL, 1001},
    {"cpu_debug", 1, NULL, 1002},
    {"cpu_freq", 1, NULL, 1003},
    {"cpu_nic_latency", 1, NULL, 1004},
    {"nic_cpu_latency", 1, NULL, 1005},
    {"nic_debug", 1, NULL, 1006},
    {"nic_net_latency", 1, NULL, 1007},
    {"exec", 1, NULL, 'e'},
    {"dimension", 1, NULL, 'D'},
};



/*
** Some (local) globals
*/
static int cpu_verbose;
static int cpu_debug;
static char *cpu_freq;
static char *cpu_nic_lat;
static char *nic_cpu_lat;
static char *nic_net_lat;
static int nic_debug;



/*
** Main
*/
int
main(int argc, char *argv[])
{

int option_index= 0;
int ch, error;
int i;
int verbose;
topo_type_t topo_type;
int dimension;
int dimX, dimY, dimZ;
int num_nodes;
char *dotFname;
FILE *fp_dot;
char *sstFname;
FILE *fp_sst;
char *execFname;
char cpu_id[MAX_ID_LEN];
char nic_link_id[MAX_ID_LEN];
int num_ports;



    /* Defaults */
    error= FALSE;
    verbose= 0;
    dotFname= "";
    sstFname= "";
    execFname= "";
    topo_type= TopoNone;
    dimX= -1;
    dimY= -1;
    dimZ= -1;
    num_nodes= -1;
    num_ports= -1;
    cpu_verbose= 0;
    cpu_debug= 0;
    cpu_freq= "2.0GHz";
    cpu_nic_lat= "1ns";
    nic_cpu_lat= "1ns";
    nic_net_lat= "1ns";
    nic_debug= 0;
    dimension= -1;


    /* check command line args */
    while (1)   {
	ch= getopt_long(argc, argv, "t:x:y:z:d:s:n:e:D:", long_options, &option_index);
	if (ch == -1)   {
	    break;
	}

	switch (ch)   {
	    case 't':
		if (strcasestr("2Dmesh", optarg) != NULL)   {
		    topo_type= Mesh2D;
		} else if (strcasestr("2Dtorus", optarg) != NULL)   {
		    topo_type= Torus2D;
		} else if (strcasestr("2DxTorus", optarg) != NULL)   {
		    topo_type= Torus2Dx;
		} else if (strcasestr("2DyTorus", optarg) != NULL)   {
		    topo_type= Torus2Dy;
		} else if (strcasestr("3Dmesh", optarg) != NULL)   {
		    topo_type= Mesh3D;
		} else if (strcasestr("3Dtorus", optarg) != NULL)   {
		    topo_type= Torus3D;
		} else if (strcasestr("3DxTorus", optarg) != NULL)   {
		    topo_type= Torus3Dx;
		} else if (strcasestr("3DyTorus", optarg) != NULL)   {
		    topo_type= Torus3Dy;
		} else if (strcasestr("3DzTorus", optarg) != NULL)   {
		    topo_type= Torus3Dz;
		} else if (strcasestr("3DxyTorus", optarg) != NULL)   {
		    topo_type= Torus3Dxy;
		} else if (strcasestr("3DxzTorus", optarg) != NULL)   {
		    topo_type= Torus3Dxz;
		} else if (strcasestr("3DyzTorus", optarg) != NULL)   {
		    topo_type= Torus3Dyz;
		} else if (strcasestr("flat2Dbutterfly", optarg) != NULL)   {
		    topo_type= Flat2Dbutter;
		} else if (strcasestr("ring", optarg) != NULL)   {
		    topo_type= Ring;
		} else if (strcasestr("full", optarg) != NULL)   {
		    topo_type= Full;
		} else if (strcasestr("tree", optarg) != NULL)   {
		    topo_type= Tree;
		} else if (strcasestr("fattree", optarg) != NULL)   {
		    topo_type= FatTree;
		} else if (strcasestr("hypercube", optarg) != NULL)   {
		    topo_type= Hypercube;
		} else   {
		    fprintf(stderr, "Unknown topology: \"%s\"\n", optarg);
		    error= TRUE;
		}
		break;
	    case 'n':
		num_nodes= strtol(optarg, (char **)NULL, 0);
		if (num_nodes < 1)   {
		    fprintf(stderr, "number of nodes must be > 0\n");
		    error= TRUE;
		}
		break;
	    case 'D':
		dimension= strtol(optarg, (char **)NULL, 0);
		if (dimension < 0)   {
		    fprintf(stderr, "(Hypercube) dimension must be >= 0\n");
		    error= TRUE;
		}
		break;
	    case 'x':
		dimX= strtol(optarg, (char **)NULL, 0);
		if (dimX < 1)   {
		    fprintf(stderr, "X dimension must be > 0\n");
		    error= TRUE;
		}
		break;
	    case 'y':
		dimY= strtol(optarg, (char **)NULL, 0);
		if (dimY < 1)   {
		    fprintf(stderr, "Y dimension must be > 0\n");
		    error= TRUE;
		}
		break;
	    case 'z':
		dimZ= strtol(optarg, (char **)NULL, 0);
		if (dimZ < 1)   {
		    fprintf(stderr, "Z dimension must be > 0\n");
		    error= TRUE;
		}
		break;
	    case 'e':
		execFname= optarg;
		break;
	    case 'd':
		dotFname= optarg;
		break;
	    case 's':
		sstFname= optarg;
		break;
	    case 1001:
		cpu_debug= strtol(optarg, (char **)NULL, 0);
		break;
	    case 1002:
		cpu_verbose= strtol(optarg, (char **)NULL, 0);
		break;
	    case 1003:
		cpu_freq= optarg;
		break;
	    case 1004:
		cpu_nic_lat= optarg;
		break;
	    case 1005:
		nic_cpu_lat= optarg;
		break;
	    case 1006:
		nic_debug= strtol(optarg, (char **)NULL, 0);
		break;
	    case 1007:
		nic_net_lat= optarg;
		break;
	    case 9999:
		error= TRUE;
		break;
	    default:
		error= TRUE;
		break;
	}
    }

    if (optind < argc)   {
	error= TRUE;
	fprintf(stderr, "No additional arguments expected!\n");
    }

    /* Open the dot file for output */
    if (strcmp(dotFname, "") == 0)   {
	/* Default */
	fp_dot= NULL;
    } else if (strcmp(dotFname, "-") == 0)   {
	fp_dot= stdout;
    } else   {
	fp_dot= fopen(dotFname, "w");
	if (fp_dot == NULL)   {
	    fprintf(stderr, "Could not open output the dot file \"%s\": %s\n", dotFname, strerror(errno));
	    exit(2);
	}
    }

    /* Open the SST xml file for output */
    if (strcmp(sstFname, "") == 0)   {
	/* Default */
	fp_sst= NULL;
    } else if (strcmp(sstFname, "-") == 0)   {
	fp_sst= stdout;
    } else   {
	fp_sst= fopen(sstFname, "w");
	if (fp_sst == NULL)   {
	    fprintf(stderr, "Could not open the SST xml output file \"%s\": %s\n", sstFname, strerror(errno));
	    exit(2);
	}
    }

    if ((strcmp(execFname, "") == 0) && (fp_sst != NULL))   {
	fprintf(stderr, "Need to specify an executable name (-e)\n");
	fprintf(stderr, "%d, %d\n", (strcmp(execFname, "") == 0), (strcmp(dotFname, "") != 0));
	error= TRUE;
    }

    if (error)   {
	usage(argv);
	exit(1);
    }


    /* Set some topology-specific parameters */
    switch (topo_type)   {
	case Mesh2D:
	    num_nodes= dimX * dimY;
	    num_ports= 5;
	    break;
	case Torus2D:
	    num_nodes= dimX * dimY;
	    num_ports= 5;
	    break;
	case Torus2Dx:
	    num_nodes= dimX * dimY;
	    num_ports= 5;
	    break;
	case Torus2Dy:
	    num_nodes= dimX * dimY;
	    num_ports= 5;
	    break;
	case Mesh3D:
	    num_nodes= dimX * dimY * dimZ;
	    num_ports= 7;
	    break;
	case Torus3D:
	    num_nodes= dimX * dimY * dimZ;
	    num_ports= 7;
	    break;
	case Torus3Dx:
	    num_nodes= dimX * dimY * dimZ;
	    num_ports= 7;
	    break;
	case Torus3Dy:
	    num_nodes= dimX * dimY * dimZ;
	    num_ports= 7;
	    break;
	case Torus3Dz:
	    num_nodes= dimX * dimY * dimZ;
	    num_ports= 7;
	    break;
	case Torus3Dxz:
	    num_nodes= dimX * dimY * dimZ;
	    num_ports= 7;
	    break;
	case Torus3Dxy:
	    num_nodes= dimX * dimY * dimZ;
	    num_ports= 7;
	    break;
	case Torus3Dyz:
	    num_nodes= dimX * dimY * dimZ;
	    num_ports= 7;
	    break;
	case Flat2Dbutter:
	    num_nodes= dimX * dimY;
	    num_ports= dimX - 1 + dimY - 1 + 1;
	    break;
	case Ring:
	    num_ports= 3;
	    break;
	case Full:
	    num_ports= num_nodes + 1;
	    break;
	case Tree:
	    if (!is_pow2(num_nodes))   {
		fprintf(stderr, "Number of nodes must be power of 2!\n");
		exit(1);
	    }
	    num_ports= 3;
	    break;
	case FatTree:
	    if (!is_pow2(num_nodes))   {
		fprintf(stderr, "Number of nodes must be power of 2!\n");
		exit(1);
	    }
	    /* Max number of ports used. Make all routers the same */
	    num_ports= num_nodes;
	    break;
	case Hypercube:
	    if (dimension < 0)   {
		fprintf(stderr, "Need to specify dimension (-D) for hypercube topology\n");
		exit(1);
	    }
	    num_ports= dimension + 1;
	    num_nodes= pow2(dimension);
	    break;
	case TopoNone:
	    fprintf(stderr, "No topology specified. Use the -t or --topology option!\n");
	    fprintf(stderr, "Valid topologies are:\n");
	    fprintf(stderr, "    2Dmesh: Two dimensional mesh, no wrap-arounds\n");
	    fprintf(stderr, "    2Dtorus: Two dimensional mesh, wrap-arounds in X and Y dimension\n");
	    fprintf(stderr, "    2DXtorus: Two dimensional mesh, wrap-arounds in X dimension only\n");
	    fprintf(stderr, "    2DYtorus: Two dimensional mesh, wrap-arounds in Y dimension only\n");
	    fprintf(stderr, "    3Dmesh: Three dimensional mesh, no wrap-arounds\n");
	    fprintf(stderr, "    3Dtorus: Three dimensional mesh, wrap-arounds in X, Y, and Z dimension\n");
	    fprintf(stderr, "    3DXtorus: Three dimensional mesh, wrap-arounds in X dimension only\n");
	    fprintf(stderr, "    3DYtorus: Three dimensional mesh, wrap-arounds in Y dimension only\n");
	    fprintf(stderr, "    3DZtorus: Three dimensional mesh, wrap-arounds in Z dimension only\n");
	    fprintf(stderr, "    3DXYtorus: Three dimensional mesh, wrap-arounds in X and Y dimension\n");
	    fprintf(stderr, "    3DXZtorus: Three dimensional mesh, wrap-arounds in X and Z dimension\n");
	    fprintf(stderr, "    3DYZtorus: Three dimensional mesh, wrap-arounds in Y and Z dimension\n");
	    fprintf(stderr, "    Ring: A ring\n");
	    fprintf(stderr, "    Full: A fully connected graph\n");
	    fprintf(stderr, "    Tree: A simple binary tree with nodes at the leaves\n");
	    fprintf(stderr, "    FatTree: A binary fat tree with nodes at the leaves\n");
	    fprintf(stderr, "    Hypercube: A hypercube of dimension -D dim\n");
	    usage(argv);
	    exit(1);
	    break;
	default:
	    fprintf(stderr, "Should not get here\n");
	    break;
    }

    if (num_nodes < 1)   {
	fprintf(stderr, "Specify number of nodes > 0 using -n, or -x and -y!\n");
	exit(1);
    }

    switch (topo_type)   {
	case Mesh2D:
	    GenMesh2D(dimX, dimY, FALSE, FALSE);
	    break;
	case Torus2D:
	    GenMesh2D(dimX, dimY, TRUE, TRUE);
	    break;
	case Torus2Dx:
	    GenMesh2D(dimX, dimY, TRUE, FALSE);
	    break;
	case Torus2Dy:
	    GenMesh2D(dimX, dimY, FALSE, TRUE);
	    break;
	case Mesh3D:
	    GenMesh3D(dimX, dimY, dimZ, FALSE, FALSE, FALSE);
	    break;
	case Torus3D:
	    GenMesh3D(dimX, dimY, dimZ, TRUE, TRUE, TRUE);
	    break;
	case Torus3Dx:
	    GenMesh3D(dimX, dimY, dimZ, TRUE, FALSE, FALSE);
	    break;
	case Torus3Dy:
	    GenMesh3D(dimX, dimY, dimZ, FALSE, TRUE, FALSE);
	    break;
	case Torus3Dz:
	    GenMesh3D(dimX, dimY, dimZ, FALSE, FALSE, TRUE);
	    break;
	case Torus3Dxy:
	    GenMesh3D(dimX, dimY, dimZ, TRUE, TRUE, FALSE);
	    break;
	case Torus3Dxz:
	    GenMesh3D(dimX, dimY, dimZ, TRUE, FALSE, TRUE);
	    break;
	case Torus3Dyz:
	    GenMesh3D(dimX, dimY, dimZ, FALSE, TRUE, TRUE);
	    break;
	case Flat2Dbutter:
	    GenFlat2Dbutter(dimX, dimY);
	    break;
	case Ring:
	    GenRing(num_nodes);
	    break;
	case Full:
	    GenFull(num_nodes);
	    break;
	case Tree:
	    GenTree(num_nodes, FALSE);
	    break;
	case FatTree:
	    GenTree(num_nodes, TRUE);
	    break;
	case Hypercube:
	    GenHyper(num_nodes, dimension);
	    break;
	case TopoNone:
	    break;
	default:
	    break;
    }


    dot_header(fp_dot, topo_names[topo_type]);
    dot_body(fp_dot);
    dot_footer(fp_dot);


    /*
    ** Start the sst xml file
    */
    sst_header(fp_sst);
    sst_cpu_param(fp_sst, cpu_freq, execFname, cpu_verbose, cpu_debug, cpu_nic_lat);
    sst_nic_param_start(fp_sst, nic_debug);
    sst_nic_param_topology(fp_sst);
    sst_nic_param_end(fp_sst, nic_cpu_lat, nic_net_lat);
    sst_router_param_start(fp_sst, num_ports);
    sst_router_param_end(fp_sst);
    sst_body_start(fp_sst);


    /* One generic proc per NIC */
    for (i= 0; i < num_nodes; i++)   {
	snprintf(cpu_id, MAX_ID_LEN, "cpu%d", i);
	snprintf(nic_link_id, MAX_ID_LEN, "cpu%dnicmodel", i);
	sst_cpu_component(cpu_id, nic_link_id, 1.0, fp_sst);
    }

    sst_nics(fp_sst);
    sst_routers(fp_sst);
    sst_body_end(fp_sst);
    sst_footer(fp_sst);

    if (fp_dot)   {
	fclose(fp_dot);
    }

    if (fp_sst)   {
	fclose(fp_sst);
    }

    return 0;

}  /* end of main() */



static void
usage(char *argv[])
{

    fprintf(stderr, "Usage: %s -t topology -e exec [-n nodes] [-x dimX] [-y dimY] [-z dimZ] [-d dfname] [-s sname] [-D dim]\n", argv[0]);
    fprintf(stderr, "   --topology, -t        one of tree, fattree, ring, hypercube, full, 2Dmesh, 2DXtorus, 2Dytorus, 2Dtorus,\n");
    fprintf(stderr, "                         3Dmesh, 3Dtorus, 3DxTorus, 3DyTorus, 3DzTorus, 3DxyTorus, 3DxzTorus, 3DyzTorus,\n");
    fprintf(stderr, "                         or flat2Dbutterfly\n");
    fprintf(stderr, "   --exec, -e            GenProc executable\n");
    fprintf(stderr, "   --nodes, -n           Number of nodes for ring, tree, etc.\n");
    fprintf(stderr, "   dimX                  Size of X dimension for 2-D and 3-D meshes\n");
    fprintf(stderr, "   dimY                  Size of Y dimension for 2-D and 3-D meshes\n");
    fprintf(stderr, "   dimZ                  Size of Z dimension for 3-D meshes\n");
    fprintf(stderr, "   --dotfilename, -d     Name of file to output dot data\n");
    fprintf(stderr, "   --sstfilename, -s     Name of the SST xml output file\n");
    fprintf(stderr, "   --dimension, -D       (Hypercube) dimension\n");
    fprintf(stderr, "   --cpu_verbose         CPU verbose parameter for xml file (default %d)\n", cpu_verbose);
    fprintf(stderr, "   --cpu_debug           CPU debug parameter for xml file (default %d)\n", cpu_debug);
    fprintf(stderr, "   --cpu_freq            CPU frequency parameter for xml file (default %s)\n", cpu_freq);
    fprintf(stderr, "   --nic_debug           NIC verbose parameter for xml file (default %d)\n", nic_debug);
    fprintf(stderr, "   --cpu_nic_latency     CPU to NIC latency parameter for xml file (default %s)\n", cpu_nic_lat);
    fprintf(stderr, "   --nic_cpu_latency     NIC to CPU latency parameter for xml file (default %s)\n", nic_cpu_lat);
    fprintf(stderr, "   --nic_net_latency     NIC to net latency parameter for xml file (default %s)\n", nic_cpu_lat);

}  /* end of usage() */


static int
is_pow2(int num)
{
    if (num < 1)   {
	return FALSE;
    }
    
    if ((num & (num - 1)) == 0)   {
	return TRUE;
    }

    return FALSE;

}  /* end of is_pow2() */


static int
pow2(int dim)
{

int i;
int res;


    res= 1;
    for (i= 0; i < dim; i++)   {
	res= res * 2;
    }

    return res;

}  /* end of pow2() */

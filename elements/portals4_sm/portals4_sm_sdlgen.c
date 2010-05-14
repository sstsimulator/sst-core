/*
 * Copyright 2009-2010 Sandia Corporation. Under the terms
 * of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
 * Government retains certain rights in this software.
 *
 * Copyright (c) 2009-2010, Sandia Corporation
 * All rights reserved.
 *
 * This file is part of the SST software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
 */


#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <libgen.h>

static void
print_usage(char *argv0)
{
    char *app = basename(argv0);

    fprintf(stderr, "Usage: %s [OPTION]...\n", app);
    fprintf(stderr, "Generate SDL file for triggered CPU/NIC\n\n");
    fprintf(stderr, "Mandatory arguments to long options are mandatory for short options too.\n");
    fprintf(stderr, "  -x, --xdim=COUNT       Size of x dimension (default: 8)\n");
    fprintf(stderr, "  -y, --ydim=COUNT       Size of y dimension (default: 8)\n");
    fprintf(stderr, "  -z, --zdim=COUNT       Size of z dimension (default: 8)\n");
    fprintf(stderr, "  -r, --radix=COUNT      Radix of tree based algorithms (default: 4)\n");
    fprintf(stderr, "  -t, --timing_set=COUNT Timing set to use (default: 2)\n");
    fprintf(stderr, "      --noise_runs=COUNT Number of iterations when noise enabled (default: 0)\n");
    fprintf(stderr, "      --noise_interval=STRING Interval for noise when enabled\n");
    fprintf(stderr, "      --noise_duration=STRING Duration for noise when enabled\n");
    fprintf(stderr, "      --msg_rate=STRING  Message rate\n");
    fprintf(stderr, "      --latency=COUNT   Latency (in ns)\n");
    fprintf(stderr, "      --message_size=SIZE Size in bytes of message\n");
    fprintf(stderr, "      --chunk_size=SIZE Size in bytes of pipeline chunk\n");
    fprintf(stderr, "      --collective=STRING Collective to run (default: allreduce)\n");
    fprintf(stderr, "      --algorithm=STRING Algorithm to run (default: tree)\n");
    fprintf(stderr, "      --output=FILENAME  Output should be sent to FILENAME (default: stdout)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "NOTE: If noise_runs is non-zero, noise_interval and noise_duration\n");
    fprintf(stderr, "must be specified\n");
}


static struct option longopts[] = {
    { "xdim",           required_argument, NULL, 'x' },
    { "ydim",           required_argument, NULL, 'y' },
    { "zdim",           required_argument, NULL, 'z' },
    { "radix",          required_argument, NULL, 'r' },
    { "timing_set",     required_argument, NULL, 't' },
    { "noise_runs",     required_argument, NULL, 'n' },
    { "noise_interval", required_argument, NULL, 'i' },
    { "noise_duration", required_argument, NULL, 'd' },
    { "msg_rate",       required_argument, NULL, 'm' },
    { "latency",        required_argument, NULL, 'l' },
    { "message_size",   required_argument, NULL, 'q' },
    { "chunk_size",     required_argument, NULL, 'w' },
    { "output",         required_argument, NULL, 'o' },
    { "help",           no_argument,       NULL, 'h' },
    { "collective",     required_argument, NULL, 'c' },
    { "algorithm",      required_argument, NULL, 'a' },
    { NULL,             0,                 NULL, 0   }
};


int
main(int argc, char **argv)
{
    int i, ch, size;
    int x_count = 8, y_count = 8, z_count = 8;
    int radix = 4, timing_set = 2, noise_runs = 0;
    char *noise_interval = NULL, *noise_duration = NULL;
    char *msg_rate = "5MHz";
    int latency = 500;
    int msg_size = 1 * 1024 * 1024;
    int chunk_size = 16 * 1024;
    char *collective = "allreduce";
    char *algorithm = "tree";
    char * nic_link_latency = "150ns";
    FILE *output = stdout;
    
    while ((ch = getopt_long(argc, argv, "hx:y:z:r:", longopts, NULL)) != -1) {
        switch (ch) {
        case 'x':
            x_count = atoi(optarg);
            break;
        case 'y':
            y_count = atoi(optarg);
            break;
        case 'z':
            z_count = atoi(optarg);
            break;
        case 'r':
            radix = atoi(optarg);
            break;
        case 't':
            timing_set = atoi(optarg);
            break;
        case 'n':
            noise_runs = atoi(optarg);
            break;
        case 'i':
            noise_interval = optarg;
            break;
        case 'd':
            noise_duration = optarg;
            break;
        case 'l':
            latency = atoi(optarg);
            break;
        case 'q':
            msg_size = atoi(optarg);
            break;
        case 'w':
            chunk_size = atoi(optarg);
            break;
        case 'm':
            msg_rate = optarg;
            break;
        case 'o':
            output = fopen(optarg, "w");
            break;
        case 'h':
            print_usage(argv[0]);
            exit(0);
        case 'c':
            collective = optarg;
            break;
        case 'a':
            algorithm = optarg;
            break;
        default:
            print_usage(argv[0]);
            exit(1);
        }
    }
    argc -= optind;
    argv += optind;

    /* backward compatibility */
    if (argc == 3 || argc == 4) {
        x_count = atoi(argv[0]);
        y_count = atoi(argv[1]);
        z_count = atoi(argv[2]);
        if (argc == 4) radix = atoi(argv[3]);
    }

    if (0 != noise_runs && (NULL == noise_interval || NULL == noise_duration)) {
        print_usage(argv[0]);
        exit(1);
    }

    /* clean up so SDL file looks nice */
    if (NULL == noise_interval) noise_interval = "1kHz";
    if (NULL == noise_duration) noise_duration = "25us";

    if ( timing_set == 1 ) nic_link_latency = "100ns";
    if ( timing_set == 2 ) nic_link_latency = "200ns";
    if ( timing_set == 3 ) nic_link_latency = "250ns";
    
    size = x_count * y_count * z_count;

    fprintf(output, "<?xml version=\"1.0\"?>\n");
    fprintf(output, "\n");
    fprintf(output, "<config>\n");
    fprintf(output, "    run-mode=both\n");
    fprintf(output, "</config>\n");
    fprintf(output, "\n");
    fprintf(output, "<rtr_params>\n");
    fprintf(output, "    <clock>         500Mhz </clock>\n");
    fprintf(output, "    <debug>         no     </debug>\n");
    fprintf(output, "    <info>          no     </info>\n");
    fprintf(output, "\n");
    fprintf(output, "    <iLCBLat>       13     </iLCBLat>\n");
    fprintf(output, "    <oLCBLat>       7      </oLCBLat>\n");
    fprintf(output, "    <routingLat>    3      </routingLat>\n");
    fprintf(output, "    <iQLat>         2      </iQLat>\n");
    fprintf(output, "\n");
    fprintf(output, "    <OutputQSize_flits>       16  </OutputQSize_flits>\n");
    fprintf(output, "    <InputQSize_flits>        96  </InputQSize_flits>\n");
    fprintf(output, "    <Router2NodeQSize_flits>  512 </Router2NodeQSize_flits>\n");
    fprintf(output, "\n");
    fprintf(output, "    <network.xDimSize> %d </network.xDimSize>\n",x_count);
    fprintf(output, "    <network.yDimSize> %d </network.yDimSize>\n",y_count);
    fprintf(output, "    <network.zDimSize> %d </network.zDimSize>\n",z_count);
    fprintf(output, "\n");
    fprintf(output, "    <routing.xDateline> 0 </routing.xDateline>\n");
    fprintf(output, "    <routing.yDateline> 0 </routing.yDateline>\n");
    fprintf(output, "    <routing.zDateline> 0 </routing.zDateline>\n");
    fprintf(output, "</rtr_params>\n");
    fprintf(output, "\n");
    fprintf(output, "<cpu_params>\n");
    fprintf(output, "    <radix> %d </radix>\n",radix);
    fprintf(output, "    <timing_set> %d </timing_set>\n",timing_set);
    fprintf(output, "    <nodes> %d </nodes>\n",size);
    fprintf(output, "    <msgrate> %s </msgrate>\n", msg_rate);
    fprintf(output, "    <xDimSize> %d </xDimSize>\n",x_count);
    fprintf(output, "    <yDimSize> %d </yDimSize>\n",y_count);
    fprintf(output, "    <zDimSize> %d </zDimSize>\n",z_count);
    fprintf(output, "    <noiseRuns> %d </noiseRuns>\n", noise_runs);
    fprintf(output, "    <noiseInterval> %s </noiseInterval>\n", noise_interval);
    fprintf(output, "    <noiseDuration> %s </noiseDuration>\n", noise_duration);
    fprintf(output, "    <collective> %s </collective>\n", collective);
    fprintf(output, "    <algorithm> %s </algorithm>\n", algorithm);
    fprintf(output, "    <latency> %d </latency>\n", latency);    
    fprintf(output, "    <msg_size> %d </msg_size>\n", msg_size);    
    fprintf(output, "    <chunk_size> %d </chunk_size>\n", chunk_size);    
    fprintf(output, "</cpu_params>\n");
    fprintf(output, "\n");
    fprintf(output, "<nic_params1>\n");
    fprintf(output, "    <clock>500Mhz</clock>\n");
    fprintf(output, "    <timing_set> %d </timing_set>\n",timing_set);
    fprintf(output, "</nic_params1>\n");
    fprintf(output, "\n");
    fprintf(output, "<nic_params2>\n");
    fprintf(output, "    <info>no</info>\n");
    fprintf(output, "    <debug>no</debug>\n");
    fprintf(output, "    <dummyDebug> no </dummyDebug>\n");
    fprintf(output, "    <latency> %d </latency>\n", latency);
    fprintf(output, "</nic_params2>\n");
    fprintf(output, "\n");
    fprintf(output, "<nicLink>\n");
/*     fprintf(output, "    <lat>1ps</lat>\n"); */
    fprintf(output, "    <lat> %s </lat>\n",nic_link_latency);
    fprintf(output, "</nicLink>\n");
    fprintf(output, "\n");
    fprintf(output, "<rtrLink>\n");
    fprintf(output, "    <lat>1ns</lat>\n");
    fprintf(output, "</rtrLink>\n");
    fprintf(output, "\n");
    fprintf(output, "<sst>\n");

    for ( i = 0; i < size; ++i) {
        int x, y, z;

	z = i / (x_count * y_count);
	y = (i / x_count) % y_count;
	x = i % x_count;

	fprintf(output, "    <component id=\"%d.cpu\" >\n",i);
	fprintf(output, "        <portals4_sm.trig_cpu>\n");
	fprintf(output, "            <params include1=cpu_params>\n");
	fprintf(output, "                <id> %d </id>\n",i);
	fprintf(output, "            </params>\n");
	fprintf(output, "            <links>\n");
	fprintf(output, "                <link id=\"%d.cpu2nic\">\n",i);
	fprintf(output, "        	    <params include=nicLink>\n");
	fprintf(output, "                        <name> nic </name>\n");
	fprintf(output, "                    </params>\n");
	fprintf(output, "                </link>\n");
	fprintf(output, "            </links>\n");
	fprintf(output, "        </portals4_sm.trig_cpu>\n");
	fprintf(output, "    </component>\n");
	fprintf(output, "\n");
	fprintf(output, "    <component id=\"%d.nic\" >\n",i);
	fprintf(output, "        <portals4_sm.trig_nic>\n");
	fprintf(output, "            <params include1=nic_params1 include2=nic_params2>\n");
	fprintf(output, "                <id> %d </id>\n",i);
	fprintf(output, "            </params>\n");
	fprintf(output, "            <links>\n");
	fprintf(output, "                <link id=\"%d.cpu2nic\">\n",i);
	fprintf(output, "        	    <params include=nicLink>\n");
	fprintf(output, "                        <name> cpu </name>\n");
	fprintf(output, "                    </params>\n");
	fprintf(output, "                </link>\n");
	fprintf(output, "                <link id=\"%d.nic2rtr\">\n",i);
	fprintf(output, "        	    <params include=nicLink>\n");
	fprintf(output, "                        <name> rtr </name>\n");
	fprintf(output, "                    </params>\n");
	fprintf(output, "                </link>\n");
	fprintf(output, "            </links>\n");
	fprintf(output, "        </portals4_sm.trig_nic>\n");
	fprintf(output, "    </component>\n");
	fprintf(output, "\n");
	fprintf(output, "    <component id=\"%d.rtr\">\n",i);
	fprintf(output, "        <SS_router.SS_router>\n");
	fprintf(output, "            <params include=rtr_params>\n");
	fprintf(output, "                <id> %d </id>\n",i);
	fprintf(output, "            </params>\n");
	fprintf(output, "            <links>\n");
	fprintf(output, "                <link id=\"%d.nic2rtr\">\n",i);
	fprintf(output, "                    <params include=nicLink>\n");
	fprintf(output, "                        <name> nic </name>\n");
	fprintf(output, "                    </params>\n");
	fprintf(output, "                </link>\n");

	if ( x_count > 1 ) {
	    fprintf(output, "                <link id=\"xr2r.%d.%d.%d\">\n",y,z,(x+1)%x_count);
	    fprintf(output, "                    <params include=rtrLink>\n");
	    fprintf(output, "                        <name> xPos </name>\n");
	    fprintf(output, "                    </params>\n");
	    fprintf(output, "                </link>\n");
	    
	    fprintf(output, "                <link id=\"xr2r.%d.%d.%d\">\n",y,z,x);
	    fprintf(output, "                    <params include=rtrLink>\n");
	    fprintf(output, "                        <name> xNeg </name>\n");
	    fprintf(output, "                    </params>\n");
	    fprintf(output, "                </link>\n");
	}

	if ( y_count > 1 ) {
	    fprintf(output, "                <link id=\"yr2r.%d.%d.%d\">\n",x,z,(y+1)%y_count);
	    fprintf(output, "                    <params include=rtrLink>\n");
	    fprintf(output, "                        <name> yPos </name>\n");
	    fprintf(output, "                    </params>\n");
	    fprintf(output, "                </link>\n");
	    
	    fprintf(output, "                <link id=\"yr2r.%d.%d.%d\">\n",x,z,y);
	    fprintf(output, "                    <params include=rtrLink>\n");
	    fprintf(output, "                        <name> yNeg </name>\n");
	    fprintf(output, "                    </params>\n");
	    fprintf(output, "                </link>\n");
	}

	if ( z_count > 1 ) {
	    fprintf(output, "                <link id=\"zr2r.%d.%d.%d\">\n",x,y,(z+1)%z_count);
	    fprintf(output, "                    <params include=rtrLink>\n");
	    fprintf(output, "                        <name> zPos </name>\n");
	    fprintf(output, "                    </params>\n");
	    fprintf(output, "                </link>\n");
	    
	    fprintf(output, "                <link id=\"zr2r.%d.%d.%d\">\n",x,y,z);
	    fprintf(output, "                    <params include=rtrLink>\n");
	    fprintf(output, "                        <name> zNeg </name>\n");
	    fprintf(output, "                    </params>\n");
	    fprintf(output, "                </link>\n");
	}
	
	fprintf(output, "            </links>\n");
	fprintf(output, "        </SS_router.SS_router>\n");
	fprintf(output, "    </component>\n");

	fprintf(output, "\n");
	fprintf(output, "\n");
    }
    fprintf(output, "</sst>\n");

    return 0;
}

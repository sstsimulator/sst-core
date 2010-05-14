#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int x, y, z;

    int xs = atoi(argv[1]);
    int ys = atoi(argv[2]);
    int zs = atoi(argv[3]);

    int size = xs*ys*zs;

    int i;


    printf("<?xml version=\"1.0\"?>\n");
    printf("\n");
    printf("<config>\n");
    printf("    stopAtCycle=1000000000\n");
    printf("    run-mode=both\n");
    printf("</config>\n");
    printf("\n");
    printf("<rtr_params>\n");
    printf("    <clock>         500Mhz </clock>\n");
    printf("    <debug>         no     </debug>\n");
    printf("    <info>          no     </info>\n");
    printf("\n");
    printf("    <iLCBLat>       13     </iLCBLat>\n");
    printf("    <oLCBLat>       7      </oLCBLat>\n");
    printf("    <routingLat>    3      </routingLat>\n");
    printf("    <iQLat>         2      </iQLat>\n");
    printf("\n");
    printf("    <OutputQSize_flits>       16  </OutputQSize_flits>\n");
    printf("    <InputQSize_flits>        96  </InputQSize_flits>\n");
    printf("    <Router2NodeQSize_flits>  512 </Router2NodeQSize_flits>\n");
    printf("\n");
    printf("    <network.xDimSize> %d </network.xDimSize>\n",xs);
    printf("    <network.yDimSize> %d </network.yDimSize>\n",ys);
    printf("    <network.zDimSize> %d </network.zDimSize>\n",zs);
    printf("\n");
    printf("    <routing.xDateline> 0 </routing.xDateline>\n");
    printf("    <routing.yDateline> 0 </routing.yDateline>\n");
    printf("    <routing.zDateline> 0 </routing.zDateline>\n");
    printf("</rtr_params>\n");
    printf("\n");
    printf("<nic_params1>\n");
    printf("    <clock>500Mhz</clock>\n");
    printf("</nic_params1>\n");
    printf("\n");
    printf("<nic_params2>\n");
    printf("    <info>no</info>\n");
    printf("    <debug>no</debug>\n");
    printf("    <dummyDebug> no </dummyDebug>\n");
    printf("    <dummy.file> foobar </dummy.file>\n");
    printf("    <dummy.nodes> %d </dummy.nodes>\n",size);
    printf("</nic_params2>\n");
    printf("\n");
    printf("<nicLink>\n");
    printf("    <lat>1ps</lat>\n");
    printf("</nicLink>\n");
    printf("\n");
    printf("<rtrLink>\n");
    printf("    <lat>1ns</lat>\n");
    printf("</rtrLink>\n");
    printf("\n");
    printf("<sst>\n");

    for ( i = 0; i < size; ++i) {
	z = i / (xs * ys);
	y = (i / xs) % ys;
	x = i % xs;

	printf("    <component id=\"%d.nic\" >\n",i);
	printf("        <SS_router.test_driver>\n");
	printf("            <params include1=nic_params1 include2=nic_params2>\n");
	printf("                <id> %d </id>\n",i);
	printf("            </params>\n");
	printf("            <links>\n");
	printf("                <link id=\"%d.nic2rtr\">\n",i);
	printf("        	    <params include=nicLink>\n");
	printf("                        <name> rtr </name>\n");
	printf("                    </params>\n");
	printf("                </link>\n");
	printf("            </links>\n");
	printf("        </SS_router.test_driver>\n");
	printf("    </component>\n");
	printf("\n");
	printf("    <component id=\"%d.rtr\">\n",i);
	printf("        <SS_router.SS_router>\n");
	printf("            <params include=rtr_params>\n");
	printf("                <id> %d </id>\n",i);
	printf("            </params>\n");
	printf("            <links>\n");
	printf("                <link id=\"%d.nic2rtr\">\n",i);
	printf("                    <params include=nicLink>\n");
	printf("                        <name> nic </name>\n");
	printf("                    </params>\n");
	printf("                </link>\n");

	if ( xs > 1 ) {
	    printf("                <link id=\"xr2r.%d.%d.%d\">\n",y,z,(x+1)%xs);
	    printf("                    <params include=rtrLink>\n");
	    printf("                        <name> xPos </name>\n");
	    printf("                    </params>\n");
	    printf("                </link>\n");
	    
	    printf("                <link id=\"xr2r.%d.%d.%d\">\n",y,z,x);
	    printf("                    <params include=rtrLink>\n");
	    printf("                        <name> xNeg </name>\n");
	    printf("                    </params>\n");
	    printf("                </link>\n");
	}

	if ( ys > 1 ) {
	    printf("                <link id=\"yr2r.%d.%d.%d\">\n",x,z,(y+1)%ys);
	    printf("                    <params include=rtrLink>\n");
	    printf("                        <name> yPos </name>\n");
	    printf("                    </params>\n");
	    printf("                </link>\n");
	    
	    printf("                <link id=\"yr2r.%d.%d.%d\">\n",x,z,y);
	    printf("                    <params include=rtrLink>\n");
	    printf("                        <name> yNeg </name>\n");
	    printf("                    </params>\n");
	    printf("                </link>\n");
	}

	if ( zs > 1 ) {
	    printf("                <link id=\"zr2r.%d.%d.%d\">\n",x,y,(z+1)%zs);
	    printf("                    <params include=rtrLink>\n");
	    printf("                        <name> zPos </name>\n");
	    printf("                    </params>\n");
	    printf("                </link>\n");
	    
	    printf("                <link id=\"zr2r.%d.%d.%d\">\n",x,y,z);
	    printf("                    <params include=rtrLink>\n");
	    printf("                        <name> zNeg </name>\n");
	    printf("                    </params>\n");
	    printf("                </link>\n");
	}
	
	printf("            </links>\n");
	printf("        </SS_router.SS_router>\n");
	printf("    </component>\n");

	printf("\n");
	printf("\n");

	

/* 	printf("%d:  %d, %d, %d\n",i,x,y,z); */
    }
    
    printf("</sst>\n");


    return 0;
}

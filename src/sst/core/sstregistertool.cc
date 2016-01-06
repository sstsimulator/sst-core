
#include <cstdio>
#include <cstdlib>
#include <cstring>


void print_usage() {
	printf("sst-register <Dependency Name> (<VAR>=<VALUE>)*\n");
	printf("\n");
	printf("<Dependency Name>   : Name of the Third Party Dependency\n");
	printf("<VAR>=<VALUE>       : Configuration variable and associated value to add to registry.\n");
	printf("                      If <VAR>=<VALUE> pairs are not provided, the tool will attempt\n");
	printf("                      to auto-register $PWD/include and $PWD/lib to the name\n");
	printf("\n");
	printf("                      Example: sst-register DRAMSim CPPFLAGS=\"-I$PWD/include\"\n");
	printf("\n");
}

int main(int argc, char* argv[]) {

	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "--help") == 0 ||
			strcmp(argv[i], "-help") == 0 ||
			strcmp(argv[i], "-h") == 0) {

			print_usage();
			exit(0);
		}
	}

	if( 1 == argc ) {
		// We are in the auto-register mode so need to query directory and the
		// environment to decide if we can make progress
	} else {
		// User is telling us what to do
	}

	return 0;
}

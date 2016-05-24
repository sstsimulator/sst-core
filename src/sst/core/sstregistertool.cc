
#include "sst_config.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <climits>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "sst/core/env/envconfig.h"
#include "sst/core/env/envquery.h"

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

	if(3 < argc) {
		print_usage();
		exit(-1);
	}

	std::string groupName(argv[1]);
	std::string keyValPair(argv[2]);

	int equalsIndex = 0;

	for(equalsIndex = 0; equalsIndex < keyValPair.size(); equalsIndex++) {
		if('=' == argv[2][equalsIndex]) {
			break;
		}
	}

	std::string key   = keyValPair.substr(0, equalsIndex);
	std::string value = keyValPair.substr(equalsIndex + 1);

	SST::Core::Environment::EnvironmentConfiguration* database = new
		SST::Core::Environment::EnvironmentConfiguration();

	char* cfgPath = (char*) malloc(sizeof(char) * PATH_MAX);

	sprintf(cfgPath, SST_INSTALL_PREFIX "/etc/sst/sstsimulator.conf");
	FILE* cfgFile = fopen(cfgPath, "r+");

	if(NULL == cfgFile) {
		char* envHome = getenv("HOME");

		if(NULL == envHome) {
			sprintf(cfgPath, "~/.sst/sstsimulator.conf");
		} else {
			sprintf(cfgPath, "%s/.sst/sstsimulator.conf", envHome);
		}

		cfgFile = fopen(cfgPath, "r+");

		if(NULL == cfgFile) {
			fprintf(stderr, "Unable to open configuration at either: %s or %s, one of these files must be editable.\n",
				SST_INSTALL_PREFIX "/etc/sst/sstsimulator.conf", cfgPath);
			exit(-1);
		}
	}

	populateEnvironmentConfig(cfgFile, database, true);

	database->getGroupByName(groupName)->setValue(key, value);
	fclose(cfgFile);

	cfgFile = fopen(cfgPath, "w+");

	if(NULL == cfgFile) {
		fprintf(stderr, "Unable to open: %s for writing.\n",
			cfgPath);
		exit(-1);
	}

	database->writeTo(cfgFile);

	fclose(cfgFile);
	free(cfgPath);

	return 0;
}

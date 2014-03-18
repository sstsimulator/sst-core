#include <sst_config.h>
#include <sst/core/bootshared.h>

int main(int argc, char* argv[]) {
	int config_env = 1;
	int verbose = 0;

	for(int i = 0; i < argc; ++i) {
		if(strcmp("--no-env-config", argv[i]) == 0) {
			config_env = 0;
		} else if(strcmp("--verbose", argv[i]) == 0) {
			verbose = 1;
		}
	}

	if(verbose && config_env) {
		printf("Launching SST with automatic environment processing enabled...\n");
	}

	if(config_env) {
		boot_sst_configure_env(verbose);
	}
	
	boot_sst_executable("sstinfo.x", verbose, argv);
}
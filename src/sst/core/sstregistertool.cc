
#include "sst_config.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <climits>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

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
		print_usage();
		exit(-1);
	}

	const char* dependencyName = argv[1];
	const size_t dependencyLen = strlen(dependencyName);


	for(size_t i = 0; i < dependencyLen; i++) {
		if( ! (isdigit(dependencyName[i]) || isalpha(dependencyName[i]) || dependencyName[i] == '_') ) {
			fprintf(stderr, "Error: non alpha-numeric at index %d in dependency %s\n",
				(int) (i+1), dependencyName);
			exit(-1);
		}
	}

	if( 2 == argc ) {
		// We are in the auto-register mode so need to query directory and the
		// environment to decide if we can make progress
		char* working_directory = (char*) malloc(sizeof(char) * PATH_MAX);
		getcwd(working_directory, PATH_MAX);

		char* include_dir = (char*) malloc(sizeof(char) * PATH_MAX);
		sprintf(include_dir, "%s/include", working_directory);

		struct stat include_info;
		stat(include_dir, &include_info);

		if( S_ISDIR(include_info.st_mode) ) {
			char* lib_dir = (char*) malloc(sizeof(char) * PATH_MAX);
			sprintf(lib_dir, "%s/lib", working_directory);

			struct stat lib_info;
			stat(lib_dir, &lib_info);

			if( S_ISDIR(lib_info.st_mode) ) {
				printf("Auto-registration: \n");

				char* reg_libval = (char*) malloc(sizeof(char) * (PATH_MAX + 2));
				char* reg_incval = (char*) malloc(sizeof(char) * (PATH_MAX + 2));

				sprintf(reg_incval, "-I%s", include_dir);

				char* config_file_path = (char*) malloc(sizeof(char) * PATH_MAX);
				sprintf(config_file_path, "%s/etc/sst/sstsimulator.conf",
					SST_INSTALL_PREFIX);
				FILE* config_file = fopen(config_file_path, "a");

				fprintf(config_file, "%s_CPPFLAGS=%s\n", dependencyName, reg_incval);
				fprintf(config_file, "%s_LIBDIR=%s\n", dependencyName, lib_dir);
				fprintf(config_file, "HAVE_%s=1\n", dependencyName);
			} else {
				fprintf(stderr, "Unable to find %s and check it is a directory, cannot auto-register.\n",
					lib_dir);
				exit(-1);
			}
		} else {
			fprintf(stderr, "Unable to find %s and check it is a directory, cannot auto-register.\n",
				include_dir);
			exit(-1);
		}
	} else {
		char* config_file_path = (char*) malloc(sizeof(char) * PATH_MAX);
		int next_arg = 2;

		// User is telling us what to do
		if( 0 == strcmp(argv[next_arg], "--global") ) {
			sprintf(config_file_path, "%s/etc/sst/sstsimulator.conf",
				SST_INSTALL_PREFIX);
			next_arg++;
		} else {
			const char* user_home = getenv("HOME");
			char* sst_home_directory = (char*) malloc(sizeof(char) * PATH_MAX);

			if( NULL == user_home ) {
				sprintf(config_file_path, "~/.sst/sstsimulator.conf");
				sprintf(sst_home_directory, "~/.sst");
			} else {
				sprintf(config_file_path, "%s/.sst/sstsimulator.conf", user_home);
				sprintf(sst_home_directory, "%s/.sst", user_home);
			}

			mkdir(sst_home_directory, 0777);
		}

		FILE* config_file = fopen(config_file_path, "a");

		if( NULL != config_file ) {
			for(int i = next_arg; i < argc; i++) {
				char* name  = strtok(argv[i], "=");
				char* value = strtok(NULL, "=");

				char* long_name = (char*) malloc(sizeof(char) * 256);
				sprintf(long_name, "%s_%s", dependencyName, name);

				fprintf(config_file, "%s=%s\n", long_name, value);
			}

			fclose(config_file);
		} else {
			fprintf(stderr, "Error: unable to open: %s for writing.\n", config_file_path);
			exit(-1);
		}
	}

	return 0;
}

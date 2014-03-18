
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <errno.h>

extern char** environ;

void update_env_var(const char* name, const int verbose);
void boot_sst_configure_env(const int verbose);
void boot_sst_executable(const char* binary, const int verbose, char* argv[]);


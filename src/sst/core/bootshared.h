// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <errno.h>

extern char** environ;

void update_env_var(const char* name, const int verbose, char* argv[], const int argc);
void boot_sst_configure_env(const int verbose, char* argv[], const int argc);
void boot_sst_executable(const char* binary, const int verbose, char* argv[], const int argc);

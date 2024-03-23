// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

extern char** environ;
/**
 * @param name Host environment
 * @param verbose Logger output setting
 * @param argv Commands for executing SST
 * @param argc Unused
 */
void          update_env_var(const char* name, const int verbose, char* argv[], const int argc);
// void boot_sst_configure_env(const SST::Config& cfg);
/**
 * Updates the LD and DYLD Library paths and sets the current path for the executable.
 * @param path Current load path
 */
void          boot_sst_configure_env(const std::string& path);
/**
 * Checks and sets the install path and environment variable of SST. Handles errors in this context.
 * @param binary Update to the real binary path
 * @param verbose Logger output setting
 * @param argv Commands for executing SST
 * @param argc Unused
 */
void          boot_sst_executable(const char* binary, const int verbose, char* argv[], const int argc);

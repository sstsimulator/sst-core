// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"

#include <cstring>
#include <string>
#include <global.h>
//#include <configuration.h>
#include "fe_debug.h"

using namespace std;

#include "ppcFrontEnd/environ.h"

static int argsInit( string cfgStr, processor *proc, simAddress base,
					simAddress str_start, int len );
static int envInit( string cfgStr, processor *proc, simAddress base,
					simAddress str_start, int len );

int environInit( string cfgStr, processor *proc, simAddress base, int len )
{
    DPRINT(1,"cfgstr=%s base=%#lx len=%#x\n",cfgStr.c_str(),
                                                (unsigned long)base,len);
    if ( len < ENVIRON_SIZE ) {
	return -1;
    } 
    int offset = argsInit( cfgStr, proc, base, base + ENVIRON_ARG_STR_OFFSET,
				ENVIRON_ARG_STR_SIZE);

    if ( offset == -1 ) return -1;

    return envInit( cfgStr, proc, base + offset, base + ENVIRON_ENV_STR_OFFSET,
				ENVIRON_ENV_STR_SIZE);
}


static int argsInit( string cfgStr, processor *proc, simAddress base,
					simAddress str_start, int str_len )
{
    simAddress offset  = str_start;
    int argc = 0;
    char *ptr;

#if 0
    //string args = configuration::getStrValue( cfgStr + ":args" );
#else
    string args = "";
#endif

    ptr = (char*) args.c_str();
    DPRINT(0,"program arguments=%s\n",ptr); 

    if ( strlen( ptr ) >= (uint)str_len ) {
        return -1;
    }

    while ( 1 ) {
        int quoted=false;

        while ( isspace( *ptr ) ) ++ptr;
        if ( *ptr == 0 ) break;

        if ( *ptr == '"' ) {
	    quoted = true;
	    ++ptr;
            if ( *ptr == 0 ) break;
        }

        proc->WriteMemory32(base + (argc+1) * sizeof(simAddress), offset, false);

	int end = false;
	char c;
        while  ( ! end && ( c = *ptr) ) {

	    if ( c == '"' ) {
		if ( quoted ) {
	            c = 0;
		    quoted = false;
		    end = true;
		} else { 
		    c = *ptr++;
		    quoted = true;
		}
	    } else if ( isspace(c) && !quoted ) {   
		c = 0;
		end = true;
            }
            proc->WriteMemory8( offset++, c, false ); 
            ++ptr;
        }
	++argc;
    }    

    proc->WriteMemory32(base + (argc+1) * sizeof(simAddress), 0, false);

    proc->WriteMemory32( base, argc, false );
    DPRINT(1,"base=%#lx\n",(unsigned long) base);
   
    return (argc + 2) * sizeof(simAddress);
}
static int envInit( string cfgStr, processor *proc, simAddress base,
					simAddress str_start, int str_len )
{
    simAddress offset  = str_start;
    int argc = 0;
    char *ptr;

#if 0
    string args = configuration::getStrValue( cfgStr + ":env" );
#else
    string args = "";
#endif

    ptr = (char*) args.c_str();

    DPRINT(1,"program env=%s\n",ptr); 

    if ( strlen( ptr ) >= (uint)str_len ) {
        return -1;
    }

    while ( 1 ) {
        int quoted=false;

        while ( isspace( *ptr ) ) ++ptr;
        if ( *ptr == 0 ) break;

        if ( *ptr == '"' ) {
	    quoted = true;
	    ++ptr;
            if ( *ptr == 0 ) break;
        }

        proc->WriteMemory32(base + argc * sizeof(simAddress), offset, false);

	int end = false;
	char c;
        while  ( ! end && ( c = *ptr++) ) {

	    if ( c == '"' ) {
		if ( quoted ) {
	            c = 0;
		    quoted = false;
		    end = true;
		} else { 
		    c = *ptr++;
		    quoted = true;
		}
	    } else if ( isspace(c) && !quoted ) {   
		c = 0;
		end = true;
            }
            proc->WriteMemory8( offset++, c, false ); 
        }
	++argc;
    }    

    proc->WriteMemory32(base + argc * sizeof(simAddress), 0, false);

    return 0;
}

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

#include "sst_config.h"

#include "sst/core/configBase.h"

#include "sst/core/env/envquery.h"
#include "sst/core/warnmacros.h"

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef E_OK
#define E_OK 0
#endif

using namespace std;

namespace SST {

bool
ConfigBase::parseBoolean(const std::string& arg, bool& success, const std::string& option)
{
    success = true;

    std::string arg_lower(arg);
    std::locale loc;
    for ( auto& ch : arg_lower )
        ch = std::tolower(ch, loc);

    if ( arg_lower == "true" || arg_lower == "yes" || arg_lower == "1" || arg_lower == "on" )
        return true;
    else if ( arg_lower == "false" || arg_lower == "no" || arg_lower == "0" || arg_lower == "off" )
        return false;
    else {
        fprintf(
            stderr,
            "ERROR: Failed to parse \"%s\" as a bool for option \"%s\", "
            "please use true/false, yes/no or 1/0\n",
            arg.c_str(), option.c_str());
        exit(-1);
        return false;
    }
}


void
ConfigBase::addOption(
    struct option opt, const char* argname, const char* desc, std::function<int(const char* arg)> callback,
    std::vector<bool> annotations, std::function<std::string(void)> ext_help)
{
    // Put this into the options vector
    options.emplace_back(opt, argname, desc, callback, false, annotations, ext_help, false);

    LongOption& new_option = options.back();

    // If this is a header, we're done
    if ( new_option.header ) return;

    // Increment the number of options
    num_options++;

    // See if there is extended help
    if ( ext_help ) has_extended_help_ = true;

    // See if this is the longest option
    size_t size = 0;
    if ( new_option.opt.name != nullptr ) { size = strlen(new_option.opt.name); }
    if ( new_option.argname != "" ) { size += new_option.argname.size() + 1; }
    if ( size > longest_option ) longest_option = size;

    if ( new_option.opt.val != 0 ) {
        // Put value in short option map with the index of where
        // to find the optoin in options vector.
        short_options[(char)new_option.opt.val] = options.size() - 1;

        // short_options_string lists all the available short
        // options.  If followed by a single colon, an argument is
        // required.  If followed by two colons, an argument is
        // optonsal.  No colon means no arguments.
        short_options_string.push_back((char)new_option.opt.val);
        if ( new_option.opt.has_arg == required_argument ) { short_options_string.push_back(':'); }
        else if ( new_option.opt.has_arg == optional_argument ) {
            short_options_string.append("::");
        }
    }

    // Handle any extra help functions
    if ( ext_help ) { extra_help_map[opt.name] = ext_help; }
}

void
ConfigBase::addHeading(const char* desc)
{
    struct option     opt = { "", optional_argument, 0, 0 };
    std::vector<bool> vec;
    options.emplace_back(
        opt, "", desc, std::function<int(const char* arg)>(), true, vec, std::function<std::string(void)>(), false);
}

std::string
ConfigBase::getUsagePrelude()
{
    return "";
}

int
ConfigBase::checkArgsAfterParsing()
{
    return 0;
}

void
ConfigBase::enableDashDashSupport(std::function<int(const char* arg)> callback)
{
    dashdash_callback = callback;
}

void
ConfigBase::addPositionalCallback(std::function<int(int num, const char* arg)> callback)
{
    positional_args = callback;
}


int
ConfigBase::printUsage()
{
    if ( suppress_print_ ) return 1;

    /* Determine screen / description widths */
    uint32_t MAX_WIDTH = 80;

    struct winsize size;
    if ( ioctl(STDERR_FILENO, TIOCGWINSZ, &size) == 0 ) { MAX_WIDTH = size.ws_col; }

    if ( getenv("COLUMNS") ) {
        errno      = E_OK;
        uint32_t x = strtoul(getenv("COLUMNS"), nullptr, 0);
        if ( errno == E_OK ) MAX_WIDTH = x;
    }

    const uint32_t ann_start  = longest_option + 6;
    const uint32_t desc_start = ann_start + annotations_.size() + 2;
    const uint32_t desc_width = MAX_WIDTH - desc_start;

    /* Print usage prelude */
    fprintf(stderr, "%s", getUsagePrelude().c_str());

    /* Print info about annotations */
    if ( has_extended_help_ ) { fprintf(stderr, "\nOptions annotated with 'H' have extended help available\n"); }
    for ( size_t i = 0; i < annotations_.size(); ++i ) {
        fprintf(stderr, "%s\n", annotations_[i].help.c_str());
    }

    // Print info about annotations

    for ( auto& option : options ) {
        if ( option.header ) {
            // Just a section heading
            fprintf(stderr, "\n%s\n", option.desc.c_str());
            continue;
        }


        uint32_t npos = 0;
        // Check for short options
        if ( option.opt.val ) { npos += fprintf(stderr, "-%c ", (char)option.opt.val); }
        else {
            npos += fprintf(stderr, "   ");
        }
        npos += fprintf(stderr, "--%s", option.opt.name);
        if ( option.opt.has_arg != no_argument ) { npos += fprintf(stderr, "=%s", option.argname.c_str()); }
        // If we have already gone beyond the description start,
        // description starts on new line
        if ( npos >= ann_start ) {
            fprintf(stderr, "\n");
            npos = 0;
        }

        // Get to the start of the annotations
        while ( npos < ann_start ) {
            npos += fprintf(stderr, " ");
        }

        // Print the annotations
        // First check for extended help
        npos += fprintf(stderr, "%c", option.ext_help ? 'H' : ' ');

        // Now do the rest of the annotations
        for ( size_t i = 0; i < annotations_.size(); ++i ) {
            char c = ' ';
            if ( option.annotations.size() >= (i + 1) && option.annotations[i] ) c = annotations_[i].annotation;
            npos += fprintf(stderr, "%c", c);
        }

        const char* text = option.desc.c_str();
        while ( text != nullptr && *text != '\0' ) {
            /* Advance to offset */
            while ( npos < desc_start )
                npos += fprintf(stderr, " ");

            if ( strlen(text) <= desc_width ) {
                fprintf(stderr, "%s", text);
                break;
            }
            else {
                // Need to find the last space before we need to break
                int index = desc_width - 1;
                while ( index > 0 ) {
                    if ( text[index] == ' ' ) break;
                    index--;
                }

                int nwritten = fprintf(stderr, "%.*s\n", index, text);
                text += (nwritten);
                // Skip any extra spaces
                while ( *text == ' ' )
                    text++;
                npos = 0;
            }
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");

    return 1; /* Should not continue */
}


int
ConfigBase::printExtHelp(const std::string& option)
{
    if ( suppress_print_ ) return 1;

    if ( extra_help_map.find(option) == extra_help_map.end() ) {
        fprintf(stderr, "No additional help found for option \"%s\"\n", option.c_str());
    }
    else {
        std::function<std::string(void)>& func = extra_help_map[option];
        std::string                       help = func();
        fprintf(stderr, "%s\n", help.c_str());
    }

    return 1; /* Should not continue */
}


int
ConfigBase::parseCmdLine(int argc, char* argv[], bool ignore_unknown)
{
    if ( suppress_print_ || ignore_unknown ) {
        // Turn off printing of errors in getopt_long
        opterr = 0;
    }
    struct option sst_long_options[num_options + 2];

    // Because a zero index can mean two different things, we put in a
    // dummy so we don't ever get a zero index
    sst_long_options[0] = { "*DUMMY_ARGUMENT*", no_argument, nullptr, 0 };
    int option_map[num_options + 1];
    option_map[0] = 0;
    {
        int count = 1;

        for ( size_t i = 0; i < options.size(); ++i ) {
            LongOption& opt = options[i];
            if ( opt.header ) continue;
            option_map[count]         = i;
            sst_long_options[count++] = opt.opt;
        }
        sst_long_options[count] = { nullptr, 0, nullptr, 0 };
    }

    run_name_ = argv[0];

    int end_arg_index = 0;
    int my_argc       = argc;

    // Check to see if '--' support was requested
    if ( dashdash_callback ) {
        // Need to see if there are model-options specified after '--'.
        // If there is, we will deal with it later and just tell getopt
        // about everything before it.  getopt does not handle -- and
        // positional arguments in a sane way.
        for ( int i = 0; i < argc; ++i ) {
            if ( !strcmp(argv[i], "--") ) {
                end_arg_index = i;
                break;
            }
        }
        my_argc = end_arg_index == 0 ? argc : end_arg_index;
    }

    int ok = 0;
    while ( 0 == ok ) {
        int       option_index = 0;
        const int intC = getopt_long(my_argc, argv, short_options_string.c_str(), sst_long_options, &option_index);

        if ( intC == -1 ) /* We're done */
            break;

        const char c = static_cast<char>(intC);

        // Getopt is a bit strange it how it returns information.
        // There are three cases:

        // 1 - Unknown value:  c = '?' & option_index = 0
        // 2 - long options:  c = first letter of option & option_index = index of option in sst_long_options
        // 3 - short options: c = short option letter & option_index = 0

        // This is a dumb combination of things.  They really should
        // have return c = 0 in the long option case.  As it is,
        // there's no way to differentiate a short value from the long
        // value in index 0.  So, we added a pad above.

        // If the character returned from getopt_long is '?', then it
        // is an unknown command
        if ( c == '?' ) {
            // Unknown option
            if ( !ignore_unknown ) { ok = printUsage(); }
        }
        else if ( option_index != 0 ) {
            // Long options
            int real_index = option_map[option_index];
            if ( optarg )
                ok = options[real_index].callback(optarg);
            else
                ok = options[real_index].callback("");
            if ( ok ) options[real_index].set_cmdline = true;
        }
        else {
            // Short option
            int real_index = short_options[c];
            if ( optarg )
                ok = options[real_index].callback(optarg);
            else
                ok = options[real_index].callback("");
            if ( ok ) options[real_index].set_cmdline = true;
        }
    }

    if ( ok != 0 ) { return ok; }

    /* Handle positional arguments */
    int    pos   = optind;
    size_t count = 0;
    while ( pos < my_argc ) {
        // Make sure that we don't have too many positional arguments
        if ( !positional_args && !suppress_print_ && !ignore_unknown ) {
            fprintf(stderr, "Error: no positional arguments allowed: %s\n", argv[pos]);
            return -1;
        }

        // If we aren't ignoring unknown arguments, we'll get an error
        // above. Otherwise, just ignore all the positional arguments.
        if ( positional_args )
            ok = positional_args(count, argv[pos++]);
        else
            pos++;
    }

    // Support further additional arguments specified after -- to be
    // args to the model.
    if ( end_arg_index != 0 ) {
        for ( int i = end_arg_index + 1; i < argc; ++i ) {
            dashdash_callback(argv[i]);
        }
    }

    if ( suppress_print_ ) {
        // Turn on printing of errors in getopt_long
        opterr = 1;
    }

    // If everything parsed okay, call the check function
    if ( ok == 0 ) return checkArgsAfterParsing();
    return ok;
}


bool
ConfigBase::setOptionExternal(const string& entryName, const string& value)
{
    // NOTE: print outs in this function will not be suppressed
    for ( auto& option : options ) {
        if ( !entryName.compare(option.opt.name) ) {
            if ( option.set_cmdline ) return false;
            return option.callback(value.c_str());
        }
    }
    fprintf(stderr, "ERROR: Unknown configuration entry \"%s\"\n", entryName.c_str());
    exit(-1);
    return false;
}

bool
ConfigBase::getAnnotation(const std::string& entryName, char annotation)
{
    // Need to look for the index of the annotation
    size_t index = std::numeric_limits<size_t>::max();
    for ( size_t i = 0; i < annotations_.size(); ++i ) {
        if ( annotations_[i].annotation == annotation ) { index = i; }
    }

    if ( index == std::numeric_limits<size_t>::max() ) {
        fprintf(stderr, "ERROR: Searching for unknown annotation: '%c'\n", annotation);
        exit(-1);
    }

    // NOTE: print outs in this function will not be suppressed
    for ( auto& option : options ) {
        if ( !entryName.compare(option.opt.name) ) {
            // Check for the annotation.  If the index is not in the
            // vector, we assume false
            if ( option.annotations.size() <= index ) return false;
            return option.annotations[index];
        }
    }

    fprintf(stderr, "ERROR: Unknown configuration entry \"%s\"\n", entryName.c_str());
    exit(-1);
    return false;
}

} // namespace SST

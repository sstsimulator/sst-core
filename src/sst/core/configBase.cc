// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/configBase.h"

#include "sst/core/env/envquery.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/util/smartTextFormatter.h"
#include "sst/core/warnmacros.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef E_OK
#define E_OK 0
#endif

namespace SST {

std::string ConfigBase::currently_parsing_option = "";

namespace StandardConfigParsers {

int
nonempty_string(std::string& var, std::string arg)
{
    if ( var.empty() ) {
        fprintf(stderr, "ERROR: Option %s must not be an empty string\n", ConfigBase::currently_parsing_option.c_str());
        return -1;
    }
    var = arg;
    return 0;
}

int
append_string(std::string pre, std::string post, std::string& var, std::string arg)
{
    if ( var.empty() ) {
        var = arg;
    }
    else {
        var += pre + arg + post;
    }
    return 0;
}

int
flag_set_true(bool& var, std::string UNUSED(arg))
{
    var = true;
    return 0;
}

int
flag_set_false(bool& var, std::string UNUSED(arg))
{
    var = false;
    return 0;
}

int
flag_default_true(bool& var, std::string arg)
{
    if ( arg == "" ) {
        var = true;
    }
    else {
        try {
            var = SST::Core::from_string<bool>(arg);
        }
        catch ( std::exception& e ) {
            fprintf(stderr, "ERROR: For option \"%s\", failed to parse \"%s\" as a boolean\n",
                ConfigBase::currently_parsing_option.c_str(), arg.c_str());
            return -1;
        }
    }
    return 0;
}

int
flag_default_false(bool& var, std::string arg)
{
    if ( arg == "" ) {
        var = false;
    }
    else {
        try {
            var = SST::Core::from_string<bool>(arg);
        }
        catch ( std::exception& e ) {
            fprintf(stderr, "ERROR: For option \"%s\", failed to parse \"%s\" as a boolean\n",
                ConfigBase::currently_parsing_option.c_str(), arg.c_str());
            return -1;
        }
    }
    return 0;
}

int
wall_time_to_seconds(uint32_t& var, std::string arg)
{
    bool success = false;

    var = ConfigBase::parseWallTimeToSeconds(arg, success, ConfigBase::currently_parsing_option);

    if ( success ) return 0;
    return -1;
}

int
element_name(std::string& var, std::string arg)
{
    if ( arg.find('.') == arg.npos ) {
        var = "sst." + arg;
    }
    else {
        var = arg;
    }

    return 0;
}
} // namespace StandardConfigParsers


uint32_t
ConfigBase::parseWallTimeToSeconds(const std::string& arg, bool& success, const std::string& option)
{
    // first attempt to parse seconds only. Assume \d+[s] until it's not
    char*         pos;
    unsigned long seconds = strtoul(arg.c_str(), &pos, 10);
    while ( isspace(*pos) )
        ++pos;
    if ( *pos == 's' || *pos == 'S' ) {
        while ( isspace(*++pos) )
            ;
        if ( !*pos ) {
            if ( seconds <= UINT32_MAX ) {
                success = true;
                return seconds;
            }
        }
    }

    static const char* templates[] = { "%H:%M:%S", "%M:%S", "%S", "%Hh", "%Mm", "%Ss" };
    const size_t       n_templ     = sizeof(templates) / sizeof(templates[0]);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    struct tm res = {}; /* This warns on GCC 4.8 due to a bug in GCC */
#pragma GCC diagnostic pop
    char* p;

    for ( size_t i = 0; i < n_templ; i++ ) {
        memset(&res, '\0', sizeof(res));
        p = strptime(arg.c_str(), templates[i], &res);
        if ( p != nullptr && *p == '\0' ) {
            seconds = res.tm_sec;
            seconds += res.tm_min * 60;
            seconds += res.tm_hour * 60 * 60;
            success = true;
            return seconds;
        }
    }
    fprintf(stderr,
        "ERROR: for option \"%s\", wall time argument could not be parsed. Argument = [%s]\nValid formats are:\n",
        option.c_str(), arg.c_str());
    for ( size_t i = 0; i < n_templ; i++ ) {
        fprintf(stderr, "\t%s\n", templates[i]);
    }
    success = false;
    // Let caller handle error
    return 0;
}

void
ConfigBase::addOption(
    struct option opt, const char* argname, const char* desc, std::vector<bool> annotations, OptionDefinition* def)
{
    // Put this into the options vector
    options.emplace_back(opt, argname, desc, false, annotations, def);

    LongOption& new_option = options.back();

    // Increment the number of options
    num_options++;

    // See if there is extended help
    if ( def->ext_help ) has_extended_help_ = true;

    // See if this is the longest option
    size_t size = 0;
    if ( new_option.opt.name != nullptr ) {
        size = strlen(new_option.opt.name);
    }
    if ( new_option.argname != "" ) {
        size += new_option.argname.size() + 1;
    }
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
        if ( new_option.opt.has_arg == required_argument ) {
            short_options_string.push_back(':');
        }
        else if ( new_option.opt.has_arg == optional_argument ) {
            short_options_string.append("::");
        }
    }

    // Handle any extra help functions
    if ( def->ext_help ) {
        extra_help_map[opt.name] = def->ext_help;
    }
}

void
ConfigBase::addHeading(const char* desc)
{
    struct option     opt = { "", optional_argument, 0, 0 };
    std::vector<bool> vec;
    options.emplace_back(opt, "", desc, true, vec, nullptr);
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
    if ( ioctl(STDERR_FILENO, TIOCGWINSZ, &size) == 0 ) {
        MAX_WIDTH = size.ws_col;
    }

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
    if ( has_extended_help_ ) {
        fprintf(stderr, "\nOptions annotated with 'H' have extended help available\n");
    }
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
        if ( option.opt.val ) {
            npos += fprintf(stderr, "-%c ", (char)option.opt.val);
        }
        else {
            npos += fprintf(stderr, "   ");
        }
        npos += fprintf(stderr, "--%s", option.opt.name);
        if ( option.opt.has_arg != no_argument ) {
            npos += fprintf(stderr, "=%s", option.argname.c_str());
        }
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
        npos += fprintf(stderr, "%c", option.def->ext_help ? 'H' : '-');

        // Now do the rest of the annotations
        for ( size_t i = 0; i < annotations_.size(); ++i ) {
            char c = '-';
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
        Util::SmartTextFormatter formatter({ 2, 5, 8 }, 1);

        std::function<std::string()>& func = extra_help_map[option];
        std::string                   help = func();
        formatter.append(help);
        fprintf(stderr, "%s\n", formatter.str().c_str());
    }

    return 1; /* Should not continue */
}

void
ConfigBase::addAnnotation(const AnnotationInfo& info)
{
    annotations_.push_back(info);
}


int
ConfigBase::parseCmdLine(int argc, char* argv[], bool ignore_unknown)
{
    if ( suppress_print_ || ignore_unknown ) {
        // Turn off printing of errors in getopt_long
        opterr = 0;
    }
    auto sst_long_options = std::make_unique<option[]>(num_options + 2);

    // Because a zero index can mean two different things, we put in a
    // dummy so we don't ever get a zero index
    sst_long_options[0] = { "*DUMMY_ARGUMENT*", no_argument, nullptr, 0 };
    auto option_map     = std::make_unique<int[]>(num_options + 1);
    option_map[0]       = 0;
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

    int status = 0;
    while ( 0 == status ) {
        int       option_index = 0;
        const int intC =
            getopt_long(my_argc, argv, short_options_string.c_str(), sst_long_options.get(), &option_index);

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
            if ( !ignore_unknown ) {
                status = printUsage();
            }
        }
        else if ( option_index != 0 ) {
            // Long options
            int real_index           = option_map[option_index];
            currently_parsing_option = options[real_index].opt.name;
            if ( optarg )
                status = options[real_index].def->parse(optarg);
            else
                status = options[real_index].def->parse("");
            if ( !status ) options[real_index].def->set_cmdline = true;
        }
        else {
            // Short option
            int real_index           = short_options[c];
            currently_parsing_option = options[real_index].opt.val;
            if ( optarg )
                status = options[real_index].def->parse(optarg);
            else
                status = options[real_index].def->parse("");
            if ( !status ) options[real_index].def->set_cmdline = true;
        }
    }

    if ( status != 0 ) {
        return status;
    }

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
        if ( positional_args ) {
            status = positional_args(count, argv[pos++]);
            ++count;
        }
        else {
            pos++;
        }
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
    if ( status == 0 ) return checkArgsAfterParsing();
    return status;
}


bool
ConfigBase::setOptionExternal(const std::string& entryName, const std::string& value)
{
    // NOTE: print outs in this function will not be suppressed
    for ( auto& option : options ) {
        if ( !entryName.compare(option.opt.name) ) {
            if ( option.def->set_cmdline ) return false;
            currently_parsing_option = option.opt.name;
            return option.def->parse(value.c_str());
        }
    }
    fprintf(stderr, "ERROR: Unknown configuration entry \"%s\"\n", entryName.c_str());
    exit(-1);
    return false;
}


bool
ConfigBase::wasOptionSetOnCmdLine(const std::string& name)
{
    for ( auto& option : options ) {
        if ( !name.compare(option.opt.name) ) {
            return option.def->set_cmdline;
        }
    }
    return false;
}


bool
ConfigBase::getAnnotation(const std::string& entryName, char annotation)
{
    // Need to look for the index of the annotation
    size_t index = getAnnotationIndex(annotation);

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

size_t
ConfigBase::getAnnotationIndex(char annotation)
{
    size_t index = std::numeric_limits<size_t>::max();
    for ( size_t i = 0; i < annotations_.size(); ++i ) {
        if ( annotations_[i].annotation == annotation ) {
            index = i;
        }
    }

    return index;
}

} // namespace SST

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

#ifndef SST_CORE_CONFIGBASE_H
#define SST_CORE_CONFIGBASE_H

#include "sst/core/sst_types.h"

#include <functional>
#include <getopt.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>

/* Forward declare for Friendship */
extern int main(int argc, char** argv);

namespace SST {

/**
   Struct that holds all the getopt_long options along with the
   docuementation for the option
*/
struct LongOption
{
    struct option                       opt;
    std::string                         argname;
    std::string                         desc;
    std::function<int(const char* arg)> callback;
    bool                                header; // if true, desc is actually the header
    std::vector<bool>                   annotations;
    std::function<std::string(void)>    ext_help;
    mutable bool                        set_cmdline;

    LongOption(
        struct option opt, const char* argname, const char* desc, const std::function<int(const char* arg)>& callback,
        bool header, std::vector<bool> annotations, std::function<std::string(void)> ext_help, bool set_cmdline) :
        opt(opt),
        argname(argname),
        desc(desc),
        callback(callback),
        header(header),
        annotations(annotations),
        ext_help(ext_help),
        set_cmdline(set_cmdline)
    {}
};

struct AnnotationInfo
{
    char        annotation;
    std::string help;
};

// Macros to make defining options easier.  These must be called
// inside of a member function of a class inheriting from ConfigBase
// Nomenaclature is:

// FLAG - value is either true or false.  FLAG defaults to no arguments allowed
// ARG - value is a string.  ARG defaults to required argument
// OPTVAL - Takes an optional paramater

// longName - multicharacter name referenced using --
// shortName - single character name referenced using -
// text - help text
// func - function called if option is found
#define DEF_FLAG_OPTVAL(longName, shortName, text, func, ...) \
    addOption({ longName, optional_argument, 0, shortName }, "[BOOL]", text, func, { __VA_ARGS__ });

#define DEF_FLAG(longName, shortName, text, func, ...) \
    addOption({ longName, no_argument, 0, shortName }, "", text, func, { __VA_ARGS__ });

#define DEF_ARG(longName, shortName, argName, text, func, ...) \
    addOption({ longName, required_argument, 0, shortName }, argName, text, func, { __VA_ARGS__ });

#define DEF_ARG_OPTVAL(longName, shortName, argName, text, func, ...) \
    addOption({ longName, optional_argument, 0, shortName }, "[" argName "]", text, func, { __VA_ARGS__ });

// Macros that include extended help
#define DEF_ARG_EH(longName, shortName, argName, text, func, eh, ...) \
    addOption({ longName, required_argument, 0, shortName }, argName, text, func, { __VA_ARGS__ }, eh);


#define DEF_SECTION_HEADING(text) addHeading(text);


/**
 * Base class to parse command line options for SST Simulation
 * Configuration variables.
 *
 * NOTE: This class contains only state for parsing the command line.
 * All options will be stored in classes derived from this class.
 * This means that we don't need to be able to serialize anything in
 * this class.
 */
class ConfigBase
{
protected:
    /**
       ConfigBase constructor.  Meant to only be created by main
       function
     */
    ConfigBase(bool suppress_print) : suppress_print_(suppress_print) {}

    /**
       Default constructor used for serialization.  After
       serialization, the Config object is only used to get the values
       of set options and it can no longer parse arguments.  Given
       that, it will no longer print anything, so set suppress_print_
       to true. None of this class needs to be serialized because it
       it's state is only for parsing the arguments.
     */
    ConfigBase() : suppress_print_(true) { options.reserve(100); }


    ConfigBase(bool suppress_print, std::vector<AnnotationInfo> annotations) :
        annotations_(annotations),
        suppress_print_(suppress_print)
    {}

    /**
       Called to print the help/usage message
    */
    int printUsage();


    /**
       Called to print the extended help for an option
     */
    int printExtHelp(const std::string& option);

    /**
       Add options to the Config object.  The options will be added in
       the order they are in the input array, and across calls to the
       function.
     */
    void addOption(
        struct option opt, const char* argname, const char* desc, std::function<int(const char* arg)> callback,
        std::vector<bool> annotations, std::function<std::string(void)> ext_help = std::function<std::string(void)>());

    /**
       Adds a heading to the usage output
     */
    void addHeading(const char* desc);

    /**
       Called to get the prelude for the help/usage message
     */
    virtual std::string getUsagePrelude();

    // Function that will be called at the end of parsing so that
    // error checking can be done
    virtual int checkArgsAfterParsing();

    // Enable support for everything after -- to be passed to a
    // callback. Each arg will be passed independently to the callback
    // function
    void enableDashDashSupport(std::function<int(const char* arg)> callback);

    // Add support for positional args.  Must be added in the order the
    // args show up on the command line
    void addPositionalCallback(std::function<int(int num, const char* arg)> callback);


    /**
       Get the name of the executable being run.  This is only
       avaialable after parseCmdLine() is called.
     */
    std::string getRunName() { return run_name_; }

    /** Set a configuration string to update configuration values */
    bool setOptionExternal(const std::string& entryName, const std::string& value);

    /** Get the value of an annotation for an option */
    bool getAnnotation(const std::string& entryName, char annotation);

public:
    // Function to uniformly parse boolean values for command line
    // arguments
    static bool parseBoolean(const std::string& arg, bool& success, const std::string& option);

    virtual ~ConfigBase() {}

    /**
       Parse command-line arguments to update configuration values.

       @return Returns 0 if execution should continue.  Returns -1 if
         there was an error.  Returns 1 if run command line only asked
         for information to be print (e.g. --help or -V, for example).
     */
    int parseCmdLine(int argc, char* argv[], bool ignore_unknown = false);

    /**
       Check to see if an option was set on the command line

       @return True if option was set on command line, false
       otherwise.  Will also return false if option is unknown.
    */
    bool wasOptionSetOnCmdLine(const std::string& option);

private:
    std::vector<LongOption>                      options;
    std::map<char, int>                          short_options;
    std::string                                  short_options_string;
    size_t                                       longest_option = 0;
    size_t                                       num_options    = 0;
    std::function<int(const char* arg)>          dashdash_callback;
    std::function<int(int num, const char* arg)> positional_args;

    // Map to hold extended help function calls
    std::map<std::string, std::function<std::string(void)>> extra_help_map;

    // Annotations
    std::vector<AnnotationInfo> annotations_;

    std::string run_name_;
    bool        suppress_print_;
    bool        has_extended_help_ = false;
};

} // namespace SST

#endif // SST_CORE_CONFIGBASE_H

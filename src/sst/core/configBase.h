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

#ifndef SST_CORE_CONFIGBASE_H
#define SST_CORE_CONFIGBASE_H

#include "sst/core/from_string.h"
#include "sst/core/serialization/serializer_fwd.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <functional>
#include <getopt.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

/* Forward declare for Friendship */
extern int main(int argc, char** argv);

namespace SST {

namespace Impl {

/**
   Function used to actually do the serialization of the option values
   in the OptionDefinition* classes.  This is needed because this
   class is used in the sst bootloader (sst), sst info (sst-info) and
   sst proper (sstsim.x) executables.  The first two don't link in
   the serialization library, so can't contain any serialization
   calls.  The function is implemented in two different places; one
   has the serialization code (configBaseSer.cc) and the other is just
   an empty function (configBaseSerEmpty.cc).

   Because the function is templated, the .cc files have to implement
   the function for every type that is used in the respective Config
   classes.  If new types are used, then the implementation of that
   type must be added to the appropriate file.
 */
template <typename T>
void option_serialize_data(SST::Core::Serialization::serializer& ser, T& val);

} // namespace Impl

/**
   Base class for defining options available in the various config
   classes.  This class and its children encapsulates the value,
   parsing function, extended help function and name.  There are also
   virtual functions that allow the Config* classes to operate on the
   options without having to know the type of the underlying data.
 */
struct OptionDefinition
{

    OptionDefinition(std::function<std::string()> ext_help) :
        ext_help(ext_help)
    {}

    // Extended help function
    const std::function<std::string()> ext_help;
    bool                               set_cmdline = false;

    virtual ~OptionDefinition() = default;

    /**
       Function called to parse the option. This generally defers to
       an underlying std::function that is defined when the
       OptionDefinition is declared (see children classes). There are
       standard parsers found in the StandardConfigParsers namespace,
       and extra parsing functions can be added to the namespace by
       child classes, if needed.

       @param arg Argument passed to the option on the command line
     */
    virtual int parse(std::string arg) = 0;

    /**
       Function called to transfer the value(s) of a separate
       OptionDefinition to the current OptionDefinition

       @param def OptionDefinition to copy value(s) from
    */
    virtual void transfer(OptionDefinition* def) = 0;

    /**
       Function called to serialize the data in the OptionDefinition.

       @see Impl::option_serialize_data()

       @param ser Serializer used to serialize the data
     */
    virtual void serialize(SST::Core::Serialization::serializer& ser) = 0;

    /**
       Adds a string representation to the input vector for each of
       the underlying variables.  Format is variable = value.  If
       there are no underlying variables, nothing will be added.

       @param[out] out vector to put the output strings in
     */
    virtual void to_string(std::vector<std::string>& out) = 0;
};

/**
   OptionDefinition representing options that don't require a
   value. This is generally used for options that only print
   infomation when specified (i.e. --help or --version), but can also
   be used if it is modifying a variable set directly in the
   containing class.
 */
struct OptionDefinitionNoVar : OptionDefinition
{
    /**
       std::function that holds a pointer to the function that will
       either print out the information, or that is modifying a
       variable contained in the enclosing class.
     */
    const std::function<int(std::string)> operate;

    /**
       Constructor

       @param operatate Function that will be called when this Option
       is called on the command line
     */
    OptionDefinitionNoVar(std::function<int(std::string)> operate) :
        OptionDefinition(nullptr),
        operate(operate)
    {}

    int  parse(std::string arg) override { return operate(arg); }
    void transfer(OptionDefinition* UNUSED(def)) override { /* no data to transfer */ }
    void serialize(SST::Core::Serialization::serializer& UNUSED(ser)) override { /* no data serialize */ }
    void to_string(std::vector<std::string>& UNUSED(out)) override {}
};

/**
   Class use to represent OptionDefinitions that store a value.  The
   class is templated on the type of the stored data.

   NOTE: This class should be created using the DECL_OPTION macro,
   which will also create an accessor function for the underlying
   data
 */
template <typename T>
struct OptionDefinitionImpl : OptionDefinition
{
    // Data members

    // Data for the option
    T                                         value = T();
    // Name of the variable as seen by the users of the class
    std::string                               name;
    // Function used to parse the data
    const std::function<int(T&, std::string)> parser;

    /**
       Constructor

       @param name Name of the option as seen by users of the class

       @param val Initialization value of the underlying value

       @param paraser Function used to parse the value of the option
    */
    OptionDefinitionImpl(std::string name, T val, std::function<int(T&, std::string)> parser) :
        OptionDefinition(nullptr),
        value(val),
        name(name),
        parser(parser)
    {}

    /**
       Constructor

       @param name Name of the option as seen by users of the class

       @param val Initialization value of the underlying value

       @param parser Function used to parse the value of the option

       @param ext_help Function called to print extended help for the
       option
    */
    OptionDefinitionImpl(
        std::string name, T val, std::function<int(T&, std::string)> parser, std::function<std::string()> ext_help) :
        OptionDefinition(ext_help),
        value(val),
        name(name),
        parser(parser)
    {}

    // Function overloads.  This makes the OptionDefinitionImpl look
    // like a type T for assignments.  Assigning one
    // OptionDefinitionImpl to another will only copy the value.
    OptionDefinitionImpl& operator=(const T& val)
    {
        value = val;
        return *this;
    }

    OptionDefinitionImpl& operator=(const OptionDefinitionImpl& val)
    {
        value = val.value;
        return *this;
    }

    // Casting the OptionDefinition as type T will just return the
    // value
    operator T() const { return value; }

    // Delete the copy and move constructors
    OptionDefinitionImpl(const OptionDefinitionImpl&) = delete;
    OptionDefinitionImpl(OptionDefinitionImpl&&)      = default;


    // Functions overriden from the base class
    int parse(std::string arg) override { return parser(value, arg); }

    void transfer(OptionDefinition* def) override { value = dynamic_cast<OptionDefinitionImpl<T>*>(def)->value; }

    void serialize(SST::Core::Serialization::serializer& ser) override { Impl::option_serialize_data(ser, value); }

    void to_string(std::vector<std::string>& out) override
    {
        std::stringstream s;
        if constexpr ( std::is_same_v<T, std::string> ) {
            s << name << "_ = " << "\"" << value << "\"";
        }
        else {
            s << name << "_ = " << value;
        }
        out.push_back(s.str());
    }
};


/**
   Class use to represent OptionDefinitions that need to store two
   values.  The class is templated on the types of the stored data.

   NOTE: This class should be created using the DECL_OPTION_PAIR
   macro, which will also create accessor functions the two underlying
   data types.
 */
template <typename T, typename U>
struct OptionDefinitionPair : OptionDefinition
{
    // Data members

    // First value
    T                                             value1 = T();
    // Name of first value as seen by users of the class
    std::string                                   name1;
    // Second value
    U                                             value2 = U();
    // Name of the second value as seen by users of the class
    std::string                                   name2;
    // Function used to parse the value from the string provided on
    // the command line
    const std::function<int(T&, U&, std::string)> parser;

    /**
       Constructor

       @param name1 Name of the first value

       @param val1 Initialzation value of value1

       @param name2 Name of the first value

       @param val2 Initialzation value of value2

       @param parser Function used to parese the values
    */
    OptionDefinitionPair(
        std::string name1, T val1, std::string name2, U val2, std::function<int(T&, U&, std::string)> parser) :
        OptionDefinition(nullptr),
        value1(val1),
        name1(name1),
        value2(val2),
        name2(name2),
        parser(parser)
    {}

    /**
       Constructor

       @param name1 Name of the first value

       @param val1 Initialzation value of value1

       @param name2 Name of the first value

       @param val2 Initialzation value of value2

       @param parser Function used to parese the values

       @param ext_help Function called to print the extended help for
       the option
    */
    OptionDefinitionPair(std::string name1, T val1, std::string name2, U val2,
        std::function<int(T&, U&, std::string)> parser, std::function<std::string()> ext_help) :
        OptionDefinition(ext_help),
        value1(val1),
        name1(name1),
        value2(val2),
        name2(name2),
        parser(parser)
    {}

    // Deleate the copy and move constructors
    OptionDefinitionPair(const OptionDefinitionPair&) = delete;
    OptionDefinitionPair(OptionDefinitionPair&&)      = default;

    // Functions overridden from the base class
    int  parse(std::string arg) override { return parser(value1, value2, arg); }
    void transfer(OptionDefinition* def) override
    {
        value1 = dynamic_cast<OptionDefinitionPair<T, U>*>(def)->value1;
        value2 = dynamic_cast<OptionDefinitionPair<T, U>*>(def)->value2;
    }

    void serialize(SST::Core::Serialization::serializer& ser) override
    {
        Impl::option_serialize_data(ser, value1);
        Impl::option_serialize_data(ser, value2);
    }

    void to_string(std::vector<std::string>& out) override
    {
        std::stringstream s;
        if constexpr ( std::is_same_v<T, std::string> ) {
            s << name1 << "_ = " << "\"" << value1 << "\"";
        }
        else {
            s << name1 << "_ = " << value1;
        }
        out.push_back(s.str());

        s.str("");
        s.clear();
        if constexpr ( std::is_same_v<U, std::string> ) {
            s << name2 << "_ = " << "\"" << value2 << "\"";
        }
        else {
            s << name2 << "_ = " << value2;
        }
        out.push_back(s.str());
    }
};

/****  Macros used to create the OptionDefinition objects ****/

// Function used to concatenate two values
#define SST_CONFIG_APPEND(first, second) first##second

/**
   Defines and initializes an OptionDefinitionImpl object. It will
   also create an accessor function for the underlying data.

   @param type Type of the underlying data

   @param name Name used to access the underlying data.  The macro
   will create a public accessor function name() that can be used to
   get the value.  It will also create an OptionDefinitionImpl object
   named name_, name_ is what is passed into the addOption() call to
   add the option to the options vector.

   @param default_val Value that the underlying value will be
   initialized to

   NOTE: the following parameters are passed though the macro using
   ... because they can contain commas and there can be either one or
   two options.  They are both function pointers. For non-member
   functions or static member functions where you don't need to bind
   one of the input parameters, you can just use &class::func or &func
   notation to pass the pointer.  For non-static member functions or
   for functions where you need to bind a parameter, you will need to
   use std::bind.

   @param parser Parsing function to be used when the option is found
   on the command line.

   @param ext_help (optional) Function called to print the extended
   help for the option.

   Note: By convention, this macro should be put in a private section
   of the class. It will declare the accessor function as public, but
   will declare the OptionDefinitionImpl as private and leave the
   class in a private section.
*/
#define SST_CONFIG_DECLARE_OPTION(type, name, default_val, ...) \
                                                                \
public:                                                         \
    type name() const                                           \
    {                                                           \
        return SST_CONFIG_APPEND(name, _).value;                \
    }                                                           \
                                                                \
private:                                                        \
    OptionDefinitionImpl<type> SST_CONFIG_APPEND(name, _) = { #name, default_val, ##__VA_ARGS__ };


/**
   Defines and initializes an OptionDefinitionPair object. It will
   also create accessor functions for both of the underlying values.

   @param type1 Type of the first value

   @param name1 Name used to access the first value.  The macro will
   create a public accessor function name1() that can be used to get
   the value.  It will also create an OptionDefinitionPair object
   named name1_, name1_ is what is passed into the addOption() call to
   add the option to the options vector.

   @param default_val1 Value that the first underlying value will be
   initialized to

   @param type2 Type of the second value

   @param name2 Name used to access the second value.  The macro
   will create a public accessor function name2() that can be used to
   get the value.

   @param default_val2 Value that the second underlying value will be
   initialized to

   NOTE: the following parameters are passed though the macro using
   ... because they can contain commas and there can be either one or
   two options.  They are both function pointers. For non-member
   functions or static member functions where you don't need to bind
   one of the input parameters, you can just use &class::func or &
   func notation to pass the pointer.  For non-static member functions
   or for functions where you need to bind a parameter, you will need
   to use std::bind.

   @param parser Parsing function to be used when the option is found
   on the command line.

   @param ext_help (optional) Function called to print the extended
   help for the option.

   Note: By convention, this macro should be put in a private section
   of the class. It will declare the accessor function as public, but
   will declare the OptionDefinitionImpl as private and leave the
   class in a private section.
*/
#define SST_CONFIG_DECLARE_OPTION_PAIR(type1, name1, default_val1, type2, name2, default_val2, ...) \
                                                                                                    \
public:                                                                                             \
    type1 name1() const                                                                             \
    {                                                                                               \
        return SST_CONFIG_APPEND(name1, _).value1;                                                  \
    }                                                                                               \
                                                                                                    \
    type2 name2() const                                                                             \
    {                                                                                               \
        return SST_CONFIG_APPEND(name1, _).value2;                                                  \
    }                                                                                               \
                                                                                                    \
private:                                                                                            \
    OptionDefinitionPair<type1, type2> SST_CONFIG_APPEND(                                           \
        name1, _) = { #name1, default_val1, #name2, default_val2, ##__VA_ARGS__ };


/**
   Defines and initializes an OptionDefinitionNoVar object.

   @param name Used to generate the name of the
   OptionDefinitionNoVar object.  It will be called name_, and
   name_ should be used to reference it in the addOption() function.

   Note: By convention, this macro should be put in a private section
   of the class. It will declare the accessor function as public, but
   will declare the OptionDefinitionImpl as private and leave the
   class in a private section.
 */
#define SST_CONFIG_DECLARE_OPTION_NOVAR(name, ...) \
                                                   \
private:                                           \
    OptionDefinitionNoVar SST_CONFIG_APPEND(name, _) = { __VA_ARGS__ };


/**
   Struct that holds all the getopt_long options along with the
   documentation for the option
*/

struct LongOption
{
    struct option     opt;
    std::string       argname;     // name of the argument passed to the option
    std::string       desc;        // Short description of the option
    bool              header;      // if true, desc is actually the header
    std::vector<bool> annotations; // annotations for the options
    OptionDefinition* def;         // OptionDefinition object used for parsing, ext_help, etc

    LongOption(struct option opt, const char* argname, const char* desc, bool header, std::vector<bool> annotations,
        OptionDefinition* def) :
        opt(opt),
        argname(argname),
        desc(desc),
        header(header),
        annotations(annotations),
        def(def)
    {}
};

struct AnnotationInfo
{
    char        annotation;
    std::string help;
};

/**** Macros used for defining options ****/

/*
  Macros to make defining options easier.  These must be called inside
  of a member function of a class inheriting from ConfigBase
  Nomenaclature is:

  FLAG - value is either true or false.  FLAG defaults to no arguments allowed
  ARG - value is a string.  ARG defaults to required argument
  OPTVAL - Takes an optional parameter

  longName - multicharacter name referenced using --
  shortName - single character name referenced using -
  text - help text
  def - OptionDefinition that defines the option attributes
*/

#define DEF_FLAG_OPTVAL(longName, shortName, text, def, ...) \
    addOption({ longName, optional_argument, 0, shortName }, "[BOOL]", text, { __VA_ARGS__ }, &def);

#define DEF_FLAG(longName, shortName, text, def, ...) \
    addOption({ longName, no_argument, 0, shortName }, "", text, { __VA_ARGS__ }, &def);

#define DEF_ARG(longName, shortName, argName, text, def, ...) \
    addOption({ longName, required_argument, 0, shortName }, argName, text, { __VA_ARGS__ }, &def);

#define DEF_ARG_OPTVAL(longName, shortName, argName, text, def, ...) \
    addOption({ longName, optional_argument, 0, shortName }, "[" argName "]", text, { __VA_ARGS__ }, &def);


#define DEF_SECTION_HEADING(text) addHeading(text);


/**
   Base class to parse command line options for SST Simulation
   Configuration variables.

   NOTE: This class contains only state for parsing the command line.
   All options will be stored in classes derived from this class.
   This means that we don't need to be able to serialize anything in
   this class.
*/
class ConfigBase
{
public:
    /**
       Variable used to identify the currently parsing option
     */
    static std::string currently_parsing_option;

protected:

    /**
       Default constructor used for serialization.  After
       serialization, the Config object is only used to get the values
       of set options and it can no longer parse arguments.  Given
       that, it will no longer print anything, so set suppress_print_
       to true. None of this class needs to be serialized because it
       it's state is only for parsing the arguments.
     */
    ConfigBase() { options.reserve(100); }


    /**
       Called to print the help/usage message
    */
    int printUsage();


    /**
       Called to print the extended help for an option
     */
    int printExtHelp(const std::string& option);

    /**
       Add an annotation available to the options
     */
    void addAnnotation(const AnnotationInfo& info);

    /**
       Add options to the Config object.  The options will be added in
       the order they are in the input array, and across calls to the
       function.
     */
    void addOption(
        struct option opt, const char* argname, const char* desc, std::vector<bool> annotations, OptionDefinition* def);

    /**
       Adds a heading to the usage output
     */
    void addHeading(const char* desc);

    /**
       Called to get the prelude for the help/usage message
     */
    virtual std::string getUsagePrelude();

    /**
       Function that will be called at the end of parsing so that
       error checking can be done
    */
    virtual int checkArgsAfterParsing();

    /**
       Enable support for everything after -- to be passed to a
       callback. Each arg will be passed independently to the callback
       function
    */
    void enableDashDashSupport(std::function<int(const char* arg)> callback);

    /**
       Add support for positional args.  Must be added in the order
       the args show up on the command line
    */
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

    /**
       Get the index in the annotation vector for the given annotation
    */
    size_t getAnnotationIndex(char annotation);

public:
    /**
       Function to parse a string to wall time using one of the
       following formats:

       H:M:S, "M:S", "S", "Hh", "Mm", "Ss"

       @param arg Argument provided on the command line

       @param[out] success Set to true if string is successfully
       parsed, false otherwise

       @param option Option that is being set.  This is only used in
       error messages.

       @return wall time in seconds
     */
    static uint32_t parseWallTimeToSeconds(const std::string& arg, bool& success, const std::string& option);

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

protected:
    std::vector<LongOption> options;

    void enable_printing() { suppress_print_ = false; }

private:
    std::map<char, int>                          short_options;
    std::string                                  short_options_string;
    size_t                                       longest_option = 0;
    size_t                                       num_options    = 0;
    std::function<int(const char* arg)>          dashdash_callback;
    std::function<int(int num, const char* arg)> positional_args;

    // Map to hold extended help function calls
    std::map<std::string, std::function<std::string()>> extra_help_map;

    // Annotations
    std::vector<AnnotationInfo> annotations_;

    std::string run_name_;
    bool        suppress_print_    = true;
    bool        has_extended_help_ = false;
};

/**** Default parsing functions ****/
namespace StandardConfigParsers {

/**
   Parses the argument using SST:Core::from_string<T>
 */
template <typename T>
int
from_string(T& var, std::string arg)
{
    try {
        var = SST::Core::from_string<T>(arg);
    }
    catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: For option \"%s\", failed to parse argument: \"%s\"\n",
            ConfigBase::currently_parsing_option.c_str(), arg.c_str());
        return -1;
    }
    return 0;
}

/**
   Parses the argument using SST::Core::from_string<T>.  If there is no
   argument provided, it will use the specified default_value.
 */
template <typename T>
int
from_string_default(T& var, std::string arg, const T& default_value)
{
    if ( arg.empty() )
        var = default_value;
    else {
        try {
            var = SST::Core::from_string<T>(arg);
        }
        catch ( std::exception& e ) {
            fprintf(stderr, "ERROR: For option \"%s\", failed to parse argument: \"%s\"\n",
                ConfigBase::currently_parsing_option.c_str(), arg.c_str());
            return -1;
        }
    }
    return 0;
}

/**
   Parser will attempt to parse the argument using the from_string<T>
   standard parser, but if successful, will store the option as a
   string.
 */
template <typename T>
int
check_parse_store_string(std::string& var, std::string arg)
{
    T   check;
    int ret = from_string<T>(check, arg);
    if ( ret != 0 ) return ret;
    var = arg;
    return 0;
}

/**
   Checks to make sure the string is not empty and stores it in the
   underying variable. If a non-empty string is provided, an error
   code will be returned.
 */
int nonempty_string(std::string& var, std::string arg);

/**
   Appends the arg to the underlying string value.  The pre string
   will be prepended to arg and the post string will be appended to
   arg before being added to the underlying string.  If the underlying
   string is currently empty (i.e. this is the first call to that
   option), pre and post are ignored.
 */
int append_string(std::string pre, std::string post, std::string& var, std::string arg);

/**
   Sets the underlying bool to true; arg is ignored (i.e. option does
   not take a value)
 */
int flag_set_true(bool& var, std::string arg);

/**
   Sets the underlying bool to false; arg is ignored (i.e. option does
   not take a value)
 */
int flag_set_false(bool& var, std::string arg);

/**
   If arg is not empty, parses it as boolean using
   SST::Core::from_string<bool>.  If arg is empty, sets the underlying
   boolean to true.
 */
int flag_default_true(bool& var, std::string arg);

/**
   If arg is not empty, parses it as boolean using
   SST::Core::from_string<bool>.  If arg is empty, sets the underlying
   boolean to false.
 */
int flag_default_false(bool& var, std::string arg);

/**
   Parses arg as a wall clock time

   @see ConfigBase:: parseWallTimeToSeconds() for a list of supported
   formats for arg
*/
int wall_time_to_seconds(uint32_t& var, std::string arg);

/**
   Treats string as an element name.  If there is no . in arg, it
   prepends sst. to the arg.  This is for backward compatiblity where
   some places in core allowed built-in elements to be specified
   without the sst library name prepended.
 */
int element_name(std::string& var, std::string arg);
} // namespace StandardConfigParsers


} // namespace SST

#endif // SST_CORE_CONFIGBASE_H

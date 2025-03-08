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

///////////////////////////////////////
// Include Files per Coding Standards
///////////////////////////////////////
#include "sst_config.h"

#include "sst/core/output.h"

// Core Headers
#include "sst/core/simulation_impl.h"

// C++ System Headers
#include <atomic>
#include <cerrno>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <string>

// System Headers
#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif // HAVE_EXECINFO_H

#include "sst/core/sst_mpi.h"

namespace SST {

// Initialize The Static Member Variables
Output      Output::m_defaultObject;
std::string Output::m_sstGlobalSimFileName        = "";
std::FILE*  Output::m_sstGlobalSimFileHandle      = nullptr;
uint32_t    Output::m_sstGlobalSimFileAccessCount = 0;

std::unordered_map<std::thread::id, uint32_t> Output::m_threadMap;
int                                           Output::m_worldSize_ranks;
int                                           Output::m_worldSize_threads;
int                                           Output::m_mpiRank = 0;

Output::Output(
    const std::string& prefix, uint32_t verbose_level, uint32_t verbose_mask, output_location_t location,
    const std::string& localoutputfilename /*=""*/)
{
    m_objInitialized = false;

    init(prefix, verbose_level, verbose_mask, location, localoutputfilename);
}

Output::Output()
{
    m_objInitialized = false;

    // Set Member Variables
    m_outputPrefix = "";
    m_verboseLevel = 0;
    m_verboseMask  = 0;
    ;
    m_targetLoc = NONE;
}

void
Output::init(
    const std::string& prefix, uint32_t verbose_level, uint32_t verbose_mask, output_location_t location,
    const std::string& localoutputfilename /*=""*/)
{
    // Only initialize if the object has not yet been initialized.
    if ( false == m_objInitialized ) {

        // Set Member Variables
        m_outputPrefix             = prefix;
        m_verboseLevel             = verbose_level;
        m_verboseMask              = verbose_mask;
        m_sstLocalFileName         = localoutputfilename;
        m_sstLocalFileHandle       = nullptr;
        m_sstLocalFileAccessCount  = 0;
        m_targetFileHandleRef      = nullptr;
        m_targetFileNameRef        = nullptr;
        m_targetFileAccessCountRef = nullptr;

        setTargetOutput(location);

        m_objInitialized = true;
    }
}

Output::~Output()
{
    // Check to see if we need to close the file
    closeSSTTargetFile();
}

void
Output::setVerboseLevel(uint32_t verbose_level)
{
    m_verboseLevel = verbose_level;
}

uint32_t
Output::getVerboseLevel() const
{
    return m_verboseLevel;
}

void
Output::setVerboseMask(uint32_t verbose_mask)
{
    m_verboseMask = verbose_mask;
}

uint32_t
Output::getVerboseMask() const
{
    return m_verboseMask;
}

void
Output::setPrefix(const std::string& prefix)
{
    m_outputPrefix = prefix;
}

std::string
Output::getPrefix() const
{
    return m_outputPrefix;
}

void
Output::setOutputLocation(output_location_t location)
{
    // Check to see if we need to close the file (if we are set to FILE)
    closeSSTTargetFile();

    // Set the new target output
    setTargetOutput(location);
}

Output::output_location_t
Output::getOutputLocation() const
{
    return m_targetLoc;
}

[[noreturn]] void
Output::fatal(uint32_t line, const char* file, const char* func, int exit_code, const char* format, ...) const
{
    va_list     arg1;
    va_list     arg2;
    std::string newFmt;

    newFmt = std::string("FATAL: ") + buildPrefixString(line, file, func) + format;

    // Get the argument list
    va_start(arg1, format);
    // Always output to STDERR
    std::vfprintf(stderr, newFmt.c_str(), arg1);
    va_end(arg1);

    // Output to the target location as long is it is not NONE or
    // STDERR (prevent 2 outputs to stderr)
    if ( true == m_objInitialized && NONE != m_targetLoc && STDERR != m_targetLoc ) {
        // Print it out to the target location
        va_start(arg2, format);

        // If the target output is a file, Make sure that the file is created and opened
        openSSTTargetFile();

        // Check to make sure output location is not NONE
        // Also make sure we are not redundantly printing to screen
        // We have already printed to stderr
        if ( NONE != m_targetLoc && STDERR != m_targetLoc && STDOUT != m_targetLoc ) {
            std::vfprintf(*m_targetOutputRef, newFmt.c_str(), arg2);
        }

        va_end(arg2);
    }

    // Flush the outputs
    std::fflush(stderr);
    flush();

#ifdef HAVE_BACKTRACE
    // Back trace so we know where this happened.
    const int backtrace_max_count = 64;
    void*     backtrace_elements[backtrace_max_count];
    char**    backtrace_names;
    size_t    backtrace_count = backtrace(backtrace_elements, backtrace_max_count);
    backtrace_names           = backtrace_symbols(backtrace_elements, backtrace_max_count);

    fprintf(stderr, "SST Fatal Backtrace Information:\n");
    for ( size_t i = 0; i < backtrace_count; ++i ) {
        fprintf(stderr, "%5" PRIu64 " : %s\n", (uint64_t)i, backtrace_names[i]);
    }

    free(backtrace_names);
#else
    fprintf(stderr, "Backtrace not available on this build/platform.\n");
#endif

    Simulation_impl::emergencyShutdown();

    SST_Exit(exit_code);
}

void
Output::setFileName(const std::string& filename) /* STATIC METHOD */
{
    // This method will be called by the SST core during startup parameter
    // checking to set the output file name.  It is not intended to be called
    // by the SST components.
    //
    // NOTE: This method can be called only once

    // Make sure we do not have a empty string from the user.
    if ( 0 == filename.length() ) {
        // We need to abort here, this is an illegal condition
        fprintf(stderr, "ERROR: Output::setFileName(filename) - Parameter filename cannot be an empty string.\n");
        exit(-1);
    }

    // Set the Filename, only if it has not yet been set.
    if ( 0 == m_sstGlobalSimFileName.length() ) { m_sstGlobalSimFileName = filename; }
    else {
        fprintf(
            stderr, "ERROR: Output::setFileName() - Filename is already set to %s, and cannot be changed.\n",
            m_sstGlobalSimFileName.c_str());
        exit(-1);
    }
}

void
Output::setTargetOutput(output_location_t location)
{
    // Set the Target location
    m_targetLoc = location;

    // Figure out where we need to send the output, we do this here rather
    // than over and over in the output methods.  If set to NONE, we choose
    // stdout, but will not output any value (checked in the output methods)
    switch ( m_targetLoc ) {
    case FILE:
        // Decide if we are sending output to the System Output file or the local debug file
        if ( 0 == m_sstLocalFileName.length() ) {
            // Set the references to the Global Simulation target file info
            m_targetOutputRef          = &m_sstGlobalSimFileHandle;
            m_targetFileHandleRef      = &m_sstGlobalSimFileHandle;
            m_targetFileNameRef        = &m_sstGlobalSimFileName;
            m_targetFileAccessCountRef = &m_sstGlobalSimFileAccessCount;
        }
        else {
            // Set the references to the Local output target file info
            m_targetOutputRef          = &m_sstLocalFileHandle;
            m_targetFileHandleRef      = &m_sstLocalFileHandle;
            m_targetFileNameRef        = &m_sstLocalFileName;
            m_targetFileAccessCountRef = &m_sstLocalFileAccessCount;
        }
        // Increment the Access count for the target output file
        (*m_targetFileAccessCountRef)++;
        break;
    case STDERR:
        m_targetOutputRef = &stderr;
        break;
    case NONE:
    case STDOUT:
    default:
        m_targetOutputRef = &stdout;
        break;
    }
}

void
Output::openSSTTargetFile() const
{
    std::FILE*  handle;
    std::string tempFileName;
    char        tempBuf[256];

    if ( true == m_objInitialized ) {
        // If the target output is a file, See if the output file is created and opened
        if ( (FILE == m_targetLoc) && (nullptr == *m_targetFileHandleRef) ) {

            // Check to see if the File has not been opened.
            if ( (*m_targetFileAccessCountRef > 0) && (nullptr == *m_targetFileHandleRef) ) {
                tempFileName = *m_targetFileNameRef;

                // Append the rank to file name if MPI_COMM_WORLD is GT 1
                if ( getMPIWorldSize() > 1 ) {
                    snprintf(tempBuf, 256, "%d", getMPIWorldRank());
                    tempFileName += tempBuf;
                }

                // Now try to open the file
                handle = fopen(tempFileName.c_str(), "w");
                if ( nullptr != handle ) { *m_targetFileHandleRef = handle; }
                else {
                    // We got an error of some sort
                    fprintf(
                        stderr, "ERROR: Output::openSSTTargetFile() - Problem opening File %s - %s\n",
                        tempFileName.c_str(), strerror(errno));
                    exit(-1);
                }
            }
        }
    }
}

void
Output::closeSSTTargetFile()
{
    if ( (true == m_objInitialized) && (FILE == m_targetLoc) ) {
        // Decrement the Access count for the file
        if ( *m_targetFileAccessCountRef > 0 ) { (*m_targetFileAccessCountRef)--; }

        // If the access count is zero, and the file has been opened, then close it
        if ( (0 == *m_targetFileAccessCountRef) && (nullptr != *m_targetFileHandleRef) && (FILE == m_targetLoc) ) {
            fclose(*m_targetFileHandleRef);
        }
    }
}

std::string
Output::buildPrefixString(uint32_t line, const std::string& file, const std::string& func) const
{
    std::string rtnstring  = "";
    size_t      startindex = 0;
    size_t      findindex  = 0;
    char        tempBuf[256];

    // Scan the string for tokens
    while ( std::string::npos != findindex ) {

        // Find the next '@' from the starting index
        findindex = m_outputPrefix.find("@", startindex);

        // Check to see if we found anything
        if ( std::string::npos != findindex ) {

            // We found the @, copy the string up to this point
            rtnstring += m_outputPrefix.substr(startindex, findindex - startindex);

            // check the next character to see what we need to do
            switch ( m_outputPrefix[findindex + 1] ) {
            case 'f':
                rtnstring += file;
                startindex = findindex + 2;
                break;
            case 'l':
                snprintf(tempBuf, 256, "%d", line);
                rtnstring += tempBuf;
                startindex = findindex + 2;
                break;
            case 'p':
                rtnstring += func;
                startindex = findindex + 2;
                break;
            case 'r':
                if ( 1 == getMPIWorldSize() ) { rtnstring += ""; }
                else {
                    snprintf(tempBuf, 256, "%d", getMPIWorldRank());
                    rtnstring += tempBuf;
                }
                startindex = findindex + 2;
                break;
            case 'R':
                if ( 1 == getMPIWorldSize() ) { rtnstring += "0"; }
                else {
                    snprintf(tempBuf, 256, "%d", getMPIWorldRank());
                    rtnstring += tempBuf;
                }
                startindex = findindex + 2;
                break;
            case 'i':
                if ( 1 == getNumThreads() ) { rtnstring += ""; }
                else {
                    snprintf(tempBuf, 256, "%u", getThreadRank());
                    rtnstring += tempBuf;
                }
                startindex = findindex + 2;
                break;
            case 'I':
                snprintf(tempBuf, 256, "%u", getThreadRank());
                rtnstring += tempBuf;
                startindex = findindex + 2;
                break;
            case 'x':
                if ( getMPIWorldSize() != 1 || getNumThreads() != 1 ) {
                    snprintf(tempBuf, 256, "[%d:%u]", getMPIWorldRank(), getThreadRank());
                    rtnstring += tempBuf;
                }
                startindex = findindex + 2;
                break;
            case 'X':
                snprintf(tempBuf, 256, "[%d:%u]", getMPIWorldRank(), getThreadRank());
                rtnstring += tempBuf;
                startindex = findindex + 2;
                break;
            case 't':
                snprintf(tempBuf, 256, "%" PRIu64, Simulation_impl::getSimulation()->getCurrentSimCycle());
                rtnstring += tempBuf;
                startindex = findindex + 2;
                break;

            default:
                // This character is not one of our tokens, so just copy it
                rtnstring += "@";
                startindex = findindex + 1;
                break;
            }
        }
    }
    // copy the remainder of the string from the start index to the end.
    rtnstring += m_outputPrefix.substr(startindex);

    return rtnstring;
}

void
Output::outputprintf(
    uint32_t line, const std::string& file, const std::string& func, const char* format, va_list arg) const
{
    std::string newFmt;

    // If the target output is a file, Make sure that the file is created and opened
    openSSTTargetFile();

    // Check to make sure output location is not NONE
    if ( NONE != m_targetLoc ) {
        newFmt = buildPrefixString(line, file, func) + format;
        std::vfprintf(*m_targetOutputRef, newFmt.c_str(), arg);
        if ( FILE == m_targetLoc ) fflush(*m_targetOutputRef);
    }
}

void
Output::outputprintf(const char* format, va_list arg) const
{
    // If the target output is a file, Make sure that the file is created and opened
    openSSTTargetFile();

    // Check to make sure output location is not NONE
    if ( NONE != m_targetLoc ) {
        std::vfprintf(*m_targetOutputRef, format, arg);
        if ( FILE == m_targetLoc ) fflush(*m_targetOutputRef);
    }
}

int
Output::getMPIWorldSize() const
{
    return m_worldSize_ranks;
}

int
Output::getMPIWorldRank() const
{
    return m_mpiRank;
}

uint32_t
Output::getNumThreads() const
{
    return m_worldSize_threads;
}

uint32_t
Output::getThreadRank() const
{
    return m_threadMap[std::this_thread::get_id()];
}

void
Output::serialize_order(SST::Core::Serialization::serializer& ser)
{
    ser& m_objInitialized;
    ser& m_outputPrefix;
    ser& m_verboseLevel;
    ser& m_verboseMask;
    ser& m_targetLoc;
    ser& m_sstLocalFileName;

    if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK && m_objInitialized ) {
        // Set Member Variables
        m_sstLocalFileHandle       = nullptr;
        m_sstLocalFileAccessCount  = 0;
        m_targetFileHandleRef      = nullptr;
        m_targetFileNameRef        = nullptr;
        m_targetFileAccessCountRef = nullptr;

        setTargetOutput(m_targetLoc);
    }
}


thread_local std::vector<char> TraceFunction::indent_array(100, ' ');
thread_local int               TraceFunction::trace_level = 0;

TraceFunction::TraceFunction(uint32_t line, const char* file, const char* func, bool print_sim_info, bool activate) :
    line_(line),
    file_(file),
    function_(func),
    indent_length_(2),
    active_(activate && global_active_)
{
    if ( !active_ ) return;
    if ( print_sim_info ) {
        Simulation_impl* sim = nullptr;
        try {
            sim = Simulation_impl::getSimulation();
        }
        catch ( std::out_of_range& e ) {
            // do nothing
        }

        if ( sim ) {
            RankInfo ri = sim->getNumRanks();
            if ( ri.rank > 1 || ri.thread > 1 ) { output_obj_.init("@x (@t): " /*prefix*/, 0, 0, Output::STDOUT); }
            else {
                output_obj_.init("(@t): ", 0, 0, Output::STDOUT);
            }
        }
        else {
            output_obj_.init("" /*prefix*/, 0, 0, Output::STDOUT);
        }
    }
    else {
        output_obj_.init("" /*prefix*/, 0, 0, Output::STDOUT);
    }

    // Set up the indent
    int indent           = trace_level * indent_length_;
    indent_array[indent] = '\0';
    output_obj_.output(line_, file, func, "%s%s enter function\n", indent_array.data(), function_.c_str());
    indent_array[indent] = indent_marker_;
    fflush(stdout);
    trace_level++;
}

TraceFunction::~TraceFunction()
{
    if ( !active_ ) return;
    trace_level--;
    int indent           = trace_level * indent_length_;
    indent_array[indent] = '\0';
    output_obj_.output(
        line_, file_.c_str(), function_.c_str(), "%s%s exit function\n", indent_array.data(), function_.c_str());
    indent_array[indent] = ' ';
    fflush(stdout);
}

void
TraceFunction::output(const char* format, ...) const
{
    if ( !active_ ) return;
    // Need to add the indent
    char* buf = new char[200];

    int indent           = trace_level * indent_length_;
    indent_array[indent] = '\0';
    std::string message(indent_array.data());

    va_list args;
    va_start(args, format);
    size_t n = vsnprintf(buf, 200, format, args);
    va_end(args);

    if ( n >= 200 ) {
        // Generated string longer than buffer
        delete[] buf;
        buf = new char[n + 1];
        va_start(args, format);
        vsnprintf(buf, n + 1, format, args);
        va_end(args);
    }

    // Look for \n and print each line individually.  We do this so we
    // can put the correct indent in and so that the prefix prints
    // correctly.
    size_t start_index = 0;
    for ( size_t i = 0; i < n - 1; ++i ) {
        if ( buf[i] == '\n' ) {
            // Terminate string here, then change it back after printing
            buf[i] = '\0';
            output_obj_.outputprintf(
                line_, file_.c_str(), function_.c_str(), "%s%s\n", indent_array.data(), &buf[start_index]);
            buf[i]      = '\n';
            start_index = i + 1;
        }
    }

    // Print the rest of the string
    output_obj_.outputprintf(line_, file_.c_str(), function_.c_str(), "%s%s", indent_array.data(), &buf[start_index]);

    delete[] buf;

    indent_array[indent] = ' ';

    // output_obj_.outputprintf(line_, file_.c_str(), function_.c_str(), "%s", message.c_str());
    // Since this class is for debug, force a flush after every output
    fflush(stdout);
}

// void
// TraceFunction::output(const char* format, ...) const
// {
//     if ( !active_ ) return;
//     // Need to add the indent
//     char* buf = new char[200];

//     int indent           = trace_level * indent_length_;
//     indent_array[indent] = '\0';
//     std::string message(indent_array.data());

//     va_list args;
//     va_start(args, format);
//     size_t n = vsnprintf(buf, 200, format, args);
//     va_end(args);

//     if ( n >= 200 ) {
//         // Generated string longer than buffer
//         delete[] buf;
//         buf = new char[n + 1];
//         va_start(args, format);
//         vsnprintf(buf, n+1, format, args);
//         va_end(args);
//     }

//     // Replace all \n's with \n + indent_array to indent any new lines
//     // in the string (unless the \n is the last character in the
//     // string)
//     size_t start_index = 0;
//     for ( size_t i = 0; i < n - 1; ++i ) {
//         if ( buf[i] == '\n' ) {
//             message.append(&buf[start_index], i - start_index + 1);
//             message.append(indent_array.data());
//             start_index = i + 1;
//         }
//     }
//     message.append(&buf[start_index], n - start_index);
//     delete[] buf;

//     indent_array[indent] = ' ';

//     output_obj_.outputprintf(line_, file_.c_str(), function_.c_str(), "%s", message.c_str());
//     fflush(stdout);
// }


// Functions to check for proper environment variable to turn on output
// for TraceFunction
bool
is_trace_function_active()
{
    const char* var = getenv("SST_TRACEFUNCTION_ACTIVATE");
    if ( var ) { return true; }
    else {
        return false;
    }
}

char
get_indent_marker()
{
    const char* var = getenv("SST_TRACEFUNCTION_INDENT_MARKER");
    if ( var ) {
        if ( strlen(var) > 0 )
            return var[0];
        else
            return '|';
    }
    return ' ';
}

bool TraceFunction::global_active_ = is_trace_function_active();
char TraceFunction::indent_marker_ = get_indent_marker();
} // namespace SST

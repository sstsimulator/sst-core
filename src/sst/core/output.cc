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

///////////////////////////////////////
// Include Files per Coding Standards
///////////////////////////////////////
#include "sst_config.h"

#include "sst/core/output.h"

// C++ System Headers
#include <cinttypes>
#include <cerrno>

// System Headers
#include <execinfo.h>

// Core Headers
#include "sst/core/simulation.h"
#include <sst/core/warnmacros.h>

#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif

namespace SST {

// Initialize The Static Member Variables
Output      Output::m_defaultObject;
std::string Output::m_sstGlobalSimFileName = "";
std::FILE*  Output::m_sstGlobalSimFileHandle = 0;
uint32_t    Output::m_sstGlobalSimFileAccessCount = 0;

std::unordered_map<std::thread::id, uint32_t> Output::m_threadMap;
RankInfo Output::m_worldSize;
int Output::m_mpiRank = 0;


Output::Output(const std::string& prefix, uint32_t verbose_level,   
               uint32_t verbose_mask,output_location_t location, 
               std::string localoutputfilename /*=""*/)
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
    m_verboseMask  = 0;;
    m_targetLoc    = NONE;
}


void Output::init(const std::string& prefix, uint32_t verbose_level,  
                  uint32_t verbose_mask, output_location_t location, 
                  std::string localoutputfilename /*=""*/)
{
    // Only initialize if the object has not yet been initialized.
    if (false == m_objInitialized) {

        // Set Member Variables
        m_outputPrefix = prefix;
        m_verboseLevel = verbose_level;
        m_verboseMask  = verbose_mask;
        m_sstLocalFileName = localoutputfilename;
        m_sstLocalFileHandle = 0;
        m_sstLocalFileAccessCount = 0;
        m_targetFileHandleRef = 0;
        m_targetFileNameRef = 0;
        m_targetFileAccessCountRef = 0;
    
        setTargetOutput(location);

        m_objInitialized = true;
    }
}


Output::~Output()
{
    // Check to see if we need to close the file
    closeSSTTargetFile();
}


void Output::setVerboseLevel(uint32_t verbose_level)
{
    m_verboseLevel = verbose_level;
}


uint32_t Output::getVerboseLevel() const
{
    return m_verboseLevel;
}


void Output::setVerboseMask(uint32_t verbose_mask)
{
    m_verboseMask = verbose_mask;
}


uint32_t Output::getVerboseMask() const
{
    return m_verboseMask;
}


void Output::setPrefix(const std::string& prefix)
{
    m_outputPrefix = prefix;
}


std::string Output::getPrefix() const
{
    return m_outputPrefix;
}


void Output::setOutputLocation(output_location_t location)
{
    // Check to see if we need to close the file (if we are set to FILE)
    closeSSTTargetFile();
    
    // Set the new target output
    setTargetOutput(location);    
}


Output::output_location_t Output::getOutputLocation() const
{
    return m_targetLoc;
}


void Output::fatal(uint32_t line, const char* file, const char* func,
                   uint32_t exit_code, 
                   const char* format, ...) const
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
    if (true == m_objInitialized && NONE != m_targetLoc && STDERR != m_targetLoc) {
        // Print it out to the target location 
        va_start(arg2, format);

        // If the target output is a file, Make sure that the file is created and opened
        openSSTTargetFile();
        
        // Check to make sure output location is not NONE
        if (NONE != m_targetLoc) {
            std::vfprintf(*m_targetOutputRef, newFmt.c_str(), arg2);
        }

        va_end(arg2);
    }
    
    // Flush the outputs    
    std::fflush(stderr);
    flush();

    // Back trace so we know where this happened.
    const int backtrace_max_count = 64;
    void* backtrace_elements[backtrace_max_count];
    char** backtrace_names;
    size_t backtrace_count = backtrace(backtrace_elements, backtrace_max_count);
    backtrace_names = backtrace_symbols(backtrace_elements, backtrace_max_count);

    fprintf(stderr, "SST Fatal Backtrace Information:\n");
    for(size_t i = 0; i < backtrace_count; ++i) {
	fprintf(stderr, "%5" PRIu64 " : %s\n", (uint64_t) i, backtrace_names[i]);
    }

    free(backtrace_names);

    Simulation::emergencyShutdown();

#ifdef SST_CONFIG_HAVE_MPI
    // If MPI exists, abort
    MPI_Abort(MPI_COMM_WORLD, exit_code);
#else
    exit(exit_code);
#endif
}



void Output::setFileName(const std::string& filename)  /* STATIC METHOD */
{
    // This method will be called by the SST core during startup parameter 
    // checking to set the output file name.  It is not intended to be called 
    // by the SST components.
    //
    // NOTE: This method can be called only once  
    
    // Make sure we do not have a empty string from the user.
    if (0 == filename.length()) {
        // We need to abort here, this is an illegal condition
        fprintf(stderr, "ERROR: Output::setFileName(filename) - Parameter filename cannot be an empty string.\n");
        exit(-1);
    }
    
    // Set the Filename, only if it has not yet been set.
    if (0 == m_sstGlobalSimFileName.length()) {
        m_sstGlobalSimFileName = filename;
    } else {
        fprintf(stderr, "ERROR: Output::setFileName() - Filename is already set to %s, and cannot be changed.\n", m_sstGlobalSimFileName.c_str());
        exit(-1);
    }
}


void Output::setTargetOutput(output_location_t location)
{
    // Set the Target location
    m_targetLoc = location;
    
    // Figure out where we need to send the output, we do this here rather
    // than over and over in the output methods.  If set to NONE, we choose 
    // stdout, but will not output any value (checked in the output methods)
    switch (m_targetLoc) {
    case FILE:
        // Decide if we are sending output to the System Output file or the local debug file
        if (0 == m_sstLocalFileName.length()) {
            // Set the references to the Global Simulation target file info
            m_targetOutputRef          = &m_sstGlobalSimFileHandle;
            m_targetFileHandleRef      = &m_sstGlobalSimFileHandle;
            m_targetFileNameRef        = &m_sstGlobalSimFileName;
            m_targetFileAccessCountRef = &m_sstGlobalSimFileAccessCount;
        } else {
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


void Output::openSSTTargetFile() const
{
    std::FILE*  handle;
    std::string tempFileName;
    char        tempBuf[256];

    if (true == m_objInitialized) {
        // If the target output is a file, See if the output file is created and opened
        if ((FILE == m_targetLoc) && (0 == *m_targetFileHandleRef)) {
  
            // Check to see if the File has not been opened.
            if ((*m_targetFileAccessCountRef > 0) && (0 == *m_targetFileHandleRef)) {
                tempFileName = *m_targetFileNameRef;
                
                // Append the rank to file name if MPI_COMM_WORLD is GT 1
                if (getMPIWorldSize() > 1) {
                    sprintf(tempBuf, "%d", getMPIWorldRank());
                    tempFileName += tempBuf;
                }
                
                // Now try to open the file
                handle = fopen(tempFileName.c_str(), "w");
                if (NULL != handle){
                    *m_targetFileHandleRef = handle;
                } else {
                    // We got an error of some sort
                    fprintf(stderr, "ERROR: Output::openSSTTargetFile() - Problem opening File %s - %s\n", tempFileName.c_str(), strerror(errno));
                    exit(-1);
                }
            }
        }
    }
}


void Output::closeSSTTargetFile()
{
    if ((true == m_objInitialized) && (FILE == m_targetLoc)) {
        // Decrement the Access count for the file
        if (*m_targetFileAccessCountRef > 0) {
            (*m_targetFileAccessCountRef)--; 
        }

        // If the access count is zero, and the file has been opened, then close it
        if ((0 == *m_targetFileAccessCountRef) && 
            (0 != *m_targetFileHandleRef) &&
            (FILE == m_targetLoc)) {
            fclose (*m_targetFileHandleRef);
        }
    }
}


std::string Output::buildPrefixString(uint32_t line, const std::string& file, const std::string& func) const
{
    std::string rtnstring = "";
    size_t      startindex = 0;
    size_t      findindex = 0;
    char        tempBuf[256];

    // Scan the string for tokens 
    while(std::string::npos != findindex){
        
        // Find the next '@' from the starting index 
        findindex = m_outputPrefix.find("@", startindex);

        // Check to see if we found anything
        if (std::string::npos != findindex){
            
            // We found the @, copy the string up to this point 
            rtnstring += m_outputPrefix.substr(startindex, findindex - startindex);

            // check the next character to see what we need to do
            switch (m_outputPrefix[findindex+1])
            {
            case 'f' :
                rtnstring += file;
                startindex = findindex + 2;
                break;
            case 'l' :
                sprintf(tempBuf, "%d", line);
                rtnstring += tempBuf;
                startindex = findindex + 2;
                break;
            case 'p' :
                rtnstring += func;
                startindex = findindex + 2;
                break;
            case 'r' :
                if (1 == getMPIWorldSize()){
                    rtnstring += "";
                } else {
                    sprintf(tempBuf, "%d", getMPIWorldRank());
                    rtnstring += tempBuf;
                }
                startindex = findindex + 2;
                break;
            case 'R' :
                if (1 == getMPIWorldSize()){
                    rtnstring += "0";
                } else {
                    sprintf(tempBuf, "%d", getMPIWorldRank());
                    rtnstring += tempBuf;
                }
                startindex = findindex + 2;
                break;
            case 'i':
                if ( 1 == getNumThreads()) {
                    rtnstring += "";
                } else {
                    sprintf(tempBuf, "%u", getThreadRank());
                    rtnstring += tempBuf;
                }
                startindex = findindex + 2;
                break;
            case 'I':
                sprintf(tempBuf, "%u", getThreadRank());
                rtnstring += tempBuf;
                startindex = findindex + 2;
                break;
            case 'x':
                if ( getMPIWorldSize() != 1 || getNumThreads() != 1 ) {
                    sprintf(tempBuf, "[%d:%u]", getMPIWorldRank(), getThreadRank());
                    rtnstring += tempBuf;
                }
                startindex = findindex + 2;
                break;
            case 'X':
                sprintf(tempBuf, "[%d:%u]", getMPIWorldRank(), getThreadRank());
                rtnstring += tempBuf;
                startindex = findindex + 2;
                break;
            case 't' :
                sprintf(tempBuf, "%" PRIu64, Simulation::getSimulation()->getCurrentSimCycle());
                rtnstring += tempBuf;
                startindex = findindex + 2;
                break;

            default :
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


void Output::outputprintf(uint32_t line, const std::string &file,
                          const std::string &func, const char* format, va_list arg) const
{
    std::string newFmt;
    
    // If the target output is a file, Make sure that the file is created and opened
    openSSTTargetFile();
    
    // Check to make sure output location is not NONE
    if (NONE != m_targetLoc) {
        newFmt = buildPrefixString(line, file, func) + format;
        std::vfprintf(*m_targetOutputRef, newFmt.c_str(), arg);
        if ( FILE == m_targetLoc) fflush(*m_targetOutputRef);
    }
}


void Output::outputprintf(const char* format, va_list arg) const
{
    // If the target output is a file, Make sure that the file is created and opened
    openSSTTargetFile();
    
    // Check to make sure output location is not NONE
    if (NONE != m_targetLoc) {
        std::vfprintf(*m_targetOutputRef, format, arg);
        if ( FILE == m_targetLoc) fflush(*m_targetOutputRef);
    }
}


int Output::getMPIWorldSize() const {
    return m_worldSize.rank;
}


int Output::getMPIWorldRank() const {
    return m_mpiRank;
}

uint32_t Output::getNumThreads() const {
    return m_worldSize.thread;
}

uint32_t Output::getThreadRank() const {
    return m_threadMap[std::this_thread::get_id()];
}



TraceFunction::TraceFunction(uint32_t line, const char* file, const char* func) {
    output.init("@R, @I (@t): " /*prefix*/, 0, 0, Output::STDOUT);               
    this->line = line;
    this->file.append(file);
    function.append(func);
    rank = Simulation::getSimulation()->getRank().rank;
    thread = Simulation::getSimulation()->getRank().thread;
    output.output(line,file,func,"%s enter function\n",function.c_str());
}

TraceFunction::~TraceFunction() {
    output.output(line, file.c_str(), function.c_str(), "%s exit function\n",function.c_str());
}



} // namespace SST

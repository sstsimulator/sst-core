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



#ifndef SST_CORE_OUTPUT_H
#define SST_CORE_OUTPUT_H

#include <string.h>

// UNCOMMENT OUT THIS LINE TO ENABLE THE DEBUG METHOD -OR_
// CHOOSE THE --enable-debug OPTION DURING SST CONFIGURATION
//#define __SST_DEBUG_OUTPUT__

//This must be defined before inclusion of intttypes.h
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <cinttypes>
#include <cstdio>
#include <thread>
#include <unordered_map>

#include <stdarg.h>

#include <sst/core/rankInfo.h>

extern int main(int argc, char **argv);

namespace SST {

// MACROS TO HELP BUILD THE CALLING FUNCTIONS INFORMATION
#define CALL_INFO      __LINE__, __FILE__, __FUNCTION__

#if defined(__GNUC__) || defined(__clang__)
#define CALL_INFO_LONG __LINE__, __FILE__, __PRETTY_FUNCTION__
#else
#define CALL_INFO_LONG __LINE__, __FILE__, __FUNCTION__
#endif

/**
 * Output object provides consistent method for outputting data to
 * stdout, stderr and/or sst debug file.  All components should
 * use this class to log any information.
 */
class Output
{
public:
    /** Choice of output location
     */
    enum output_location_t {
        NONE,           /*!< No output */
        STDOUT,     /*!< Print to stdout */
        STDERR,     /*!< Print to stderr */
        FILE        /*!< Print to a file */
    };

    /** Constructor.  Set up output configuration.
        @param prefix Prefix to be prepended to all strings emitted by calls to
               debug(), verbose(), fatal() and possibly output().
               NOTE: No space will be inserted between the prepended prefix
               string and the normal output string.
               Prefix can contain the following escape codes:
               - \@f Name of the file in which output call was made.
               - \@l Line number in the file in which output call was made.
               - \@p Name of the function from which output call was made.
               - \@r MPI rank of the calling process.  Will be empty if
                  MPI_COMM_WORLD size is 1.
               - \@R MPI rank of the calling process.  Will be 0 if
                  MPI_COMM_WORLD size is 1.
               - \@i Thread Id of the calling process.  Will be empty if
                  number of threads is 1.
               - \@I Thread Id of the calling process.  Will be 0 if
                  number of threads is 1.
               - \@x Rank information of the calling process.  Will be empty if
                  number of MPI ranks and number of threads are both 1
                  Same as [\@r:\@i]
               - \@X Rank information of the calling process.  Will be [0.0] if
                  number of MPI ranks and number of threads are both 1
                  Same as [\@R:\@I]
               - \@t Simulation time.  Will be the raw simulation cycle time
                  retrieved from the SST Core.
        @param verbose_level Debugging output level.  Calls to debug(),
               verbose() and fatal() are only output if their output_level
               parameter is less than or equal to the verbose_level currently
               set for the object
        @param verbose_mask Bitmask of allowed message types for debug(),
               verbose() and fatal().  The Output object will only output the
               message if the set bits of the output_bits parameter
               are set in the verbose_mask of the object.  It uses this logic:
               if (~verbose_mask & output_bits == 0) then output is enabled.
        @param location Output location.  Output will be directed to STDOUT,
               STDERR, FILE, or NONE.  If FILE output is selected, the
               output will be directed to the file defined by the
               --debug runtime parameter, or to the file 'sst_output' if the
               --debug parameter is not defined.  If the size of MPI_COMM_WORLD
               is > 1, then the rank process will be appended to the file name.
        @param localoutputfilename.  Send the output of this class to the 
               file identified in localoutputfilename instead of the of the 
               normal output file set by the run time parameter --debug-file.  
               location parameter must be set to FILE.  This parameter is 
               intended for special case debug purposes only.
    */
    // CONSTRUCTION / DESTRUCTION
    Output(const std::string& prefix, uint32_t verbose_level,
           uint32_t verbose_mask, output_location_t location,
           std::string localoutputfilename = "");

    /** Default Constructor.  User must call init() to properly initialize obj.
        Until init() is called, no output will occur.
    */
    Output();  // Default constructor

    virtual ~Output();


    /** Initialize the object after construction
        @param prefix Prefix to be prepended to all strings emitted by calls to
               debug(), verbose(), fatal() and possibly output().
               NOTE: No space will be inserted between the prepended prefix
               string and the normal output string.
               Prefix can contain the following escape codes:
               - \@f Name of the file in which output call was made.
               - \@l Line number in the file in which output call was made.
               - \@p Name of the function from which output call was made.
               - \@r MPI rank of the calling process.  Will be empty if
                  MPI_COMM_WORLD size is 1.
               - \@R MPI rank of the calling process.  Will be 0 if
                  MPI_COMM_WORLD size is 1.
               - \@i Thread Id of the calling process.  Will be empty if
                  number of threads is 1.
               - \@I Thread Id of the calling process.  Will be 0 if
                  number of threads is 1.
               - \@x Rank information of the calling process.  Will be empty if
                  number of MPI ranks and number of threads are both 1
                  Same as [\@r:\@i]
               - \@X Rank information of the calling process.  Will be [0.0] if
                  number of MPI ranks and number of threads are both 1
                  Same as [\@R:\@I]
               - \@t Simulation time.  Will be the raw simulation cycle time
                  retrieved from the SST Core.
        @param verbose_level Debugging output level.  Calls to debug(),
               verbose() and fatal() are only output if their output_level
               parameter is less than or equal to the verbose_level currently
               set for the object
        @param verbose_mask Bitmask of allowed message types for debug(),
               verbose() and fatal().  The Output object will only output the
               message if the set bits of the output_bits parameter
               are set in the verbose_mask of the object.  It uses this logic:
               if (~verbose_mask & output_bits == 0) then output is enabled.
        @param location Output location.  Output will be directed to STDOUT,
               STDERR, FILE, or NONE.  If FILE output is selected, the
               output will be directed to the file defined by the
               --debug runtime parameter, or to the file 'sst_output' if the
               --debug parameter is not defined.  If the size of MPI_COMM_WORLD
               is > 1, then the rank process will be appended to the file name.
        @param localoutputfilename.  Send the output of this class to the 
               file identified in localoutputfilename instead of the of the 
               normal output file set by the run time parameter --debug-file.  
               location parameter must be set to FILE.  This parameter is 
               intended for special case debug purposes only.
    */
    // INITIALIZATION
    void init(const std::string& prefix, uint32_t verbose_level,
               uint32_t verbose_mask, output_location_t location,
               std::string localoutputfilename = "");

    /** Output the message with formatting as specified by the format parameter.
        The output will be prepended with the expanded prefix set in the object.
        @param line Line number of calling function (use CALL_INFO macro)
        @param file File name calling function (use CALL_INFO macro)
        @param func Function name calling function (use CALL_INFO macro)
        @param format Format string.  All valid formats for printf are available.
        @param ... Argument strings for format.
     */
    void output(uint32_t line, const char* file, const char* func,
                const char* format, ...) const
        __attribute__ ((format (printf, 5, 6)))
    {
        va_list arg;
        if (true == m_objInitialized && NONE != m_targetLoc ) {
            // Get the argument list and then print it out
            va_start(arg, format);
            outputprintf(line, file, func, format, arg);
            va_end(arg);
        }
    }

    /** Output the message with formatting as specified by the format parameter.
        @param format Format string.  All valid formats for printf are available.
        @param ... Arguments for format.
     */
    void output(const char* format, ...) const
         __attribute__ ((format (printf, 2, 3)))
    {
            va_list arg;
            if (true == m_objInitialized && NONE != m_targetLoc) {
                // Get the argument list and then print it out
                va_start(arg, format);
                outputprintf(format, arg);
                va_end(arg);
            }
    }

    /** Output the verbose message with formatting as specified by the format
        parameter. Output will only occur if specified output_level and
        output_bits meet criteria defined by object.  The output will be
        prepended with the expanded prefix set in the object.
        @param line Line number of calling function (use CALL_INFO macro)
        @param file File name calling function (use CALL_INFO macro)
        @param func Function name calling function (use CALL_INFO macro)
        @param output_level For output to occur, output_level must be less than
               or equal to verbose_level set in object
        @param output_bits The Output object will only output the
               message if the set bits of the output_bits parameter are set in
               the verbose_mask of the object.  It uses this logic:
               if (~verbose_mask & output_bits == 0) then output is enabled.
        @param format Format string.  All valid formats for printf are available.
        @param ... Arguments for format.
     */
    void verbose(uint32_t line, const char* file, const char* func,
                 uint32_t output_level, uint32_t output_bits,
                 const char* format, ...)    const
        __attribute__ ((format (printf, 7, 8)))
    {
        va_list arg;

        if (true == m_objInitialized && NONE != m_targetLoc ) {
            // First check to see if we are allowed to send output based upon the
            // verbose_mask and verbose_level checks
            if (((output_bits & ~m_verboseMask) == 0) &&
                (output_level <= m_verboseLevel)){
                // Get the argument list and then print it out
                va_start(arg, format);
                outputprintf(line, file, func, format, arg);
                va_end(arg);
            }
        }
    }

    /** Output the verbose message with formatting as specified by the format
        parameter. Output will only occur if specified output_level and
        output_bits meet criteria defined by object.  The output will be
        prepended with the expanded prefix set in the object.
        @param tempPrefix For just this call use this prefix
        @param line Line number of calling function (use CALL_INFO macro)
        @param file File name calling function (use CALL_INFO macro)
        @param func Function name calling function (use CALL_INFO macro)
        @param output_level For output to occur, output_level must be less than
               or equal to verbose_level set in object
        @param output_bits The Output object will only output the
               message if the set bits of the output_bits parameter are set in
               the verbose_mask of the object.  It uses this logic:
               if (~verbose_mask & output_bits == 0) then output is enabled.
        @param format Format string.  All valid formats for printf are available.
        @param ... Arguments for format.
     */
    void verbosePrefix(const char* tempPrefix, uint32_t line, const char* file, const char* func,
                 uint32_t output_level, uint32_t output_bits,
                 const char* format, ...)
        __attribute__ ((format (printf, 8, 9)))
    {

        va_list arg;

        if (true == m_objInitialized && NONE != m_targetLoc ) {
    	    const std::string normalPrefix = m_outputPrefix;
            m_outputPrefix = tempPrefix;

            // First check to see if we are allowed to send output based upon the
            // verbose_mask and verbose_level checks
            if (((output_bits & ~m_verboseMask) == 0) &&
                (output_level <= m_verboseLevel)){
                // Get the argument list and then print it out
                va_start(arg, format);
                outputprintf(line, file, func, format, arg);
                va_end(arg);
            }

            m_outputPrefix = normalPrefix;
        }
    }

    /** Output the debug message with formatting as specified by the format
        parameter. Output will only occur if specified output_level and
        output_bits meet criteria defined by object.  The output will be
        prepended with the expanded prefix set in the object.
        NOTE: Debug outputs will only occur if the __SST_DEBUG_OUTPUT__ is defined.
              this define can be set in source code or by setting the
              --enable-debug option during SST configuration.
        @param line Line number of calling function (use CALL_INFO macro)
        @param file File name calling function (use CALL_INFO macro)
        @param func Function name calling function (use CALL_INFO macro)
        @param output_level For output to occur, output_level must be less than
               or equal to verbose_level set in object
        @param output_bits The Output object will only output the
               message if the set bits of the output_bits parameter are set in
               the verbose_mask of the object.  It uses this logic:
               if (~verbose_mask & output_bits == 0) then output is enabled.
        @param format Format string.  All valid formats for printf are available.
        @param ... Arguments for format.
     */
    void debug(uint32_t line, const char* file, const char* func,
               uint32_t output_level, uint32_t output_bits,
               const char* format, ...)   const
        __attribute__ ((format (printf, 7, 8)))
    {
#ifdef __SST_DEBUG_OUTPUT__
        va_list arg;
        if (true == m_objInitialized && NONE != m_targetLoc ) {
            // First check to see if we are allowed to send output based upon the
            // verbose_mask and verbose_level checks
            if (((output_bits & ~m_verboseMask) == 0) &&
                (output_level <= m_verboseLevel)){
                // Get the argument list and then print it out
                va_start(arg, format);
                outputprintf(line, file, func, format, arg);
                va_end(arg);
            }
        }
#else
        /* When debug is disabled, silence warnings of unused parameters */
        (void)line;
        (void)file;
        (void)func;
        (void)output_level;
        (void)output_bits;
        (void)format;
#endif
    }


    /** Output the fatal message with formatting as specified by the format
        parameter.  Message will be sent to the output location and to stderr.
        The output will be prepended with the expanded prefix set
        in the object.
        NOTE: fatal() will call MPI_Abort(exit_code) to terminate simulation.
        @param line Line number of calling function (use CALL_INFO macro)
        @param file File name calling function (use CALL_INFO macro)
        @param func Function name calling function (use CALL_INFO macro)
        @param exit_code The exit code used for termination of simulation.
               will be passed to MPI_Abort()
        @param format Format string.  All valid formats for printf are available.
        @param ... Arguments for format.
     */
    void fatal(uint32_t line, const char* file, const char* func,
               uint32_t exit_code,
               const char* format, ...)    const
                  __attribute__ ((format (printf, 6, 7))) ;


    // GET / SET METHODS

    /** Sets object prefix
        @param prefix Prefix to be prepended to all strings emitted by calls to
               debug(), verbose(), fatal() and possibly output().
               NOTE: No space will be inserted between the prepended prefix
               string and the normal output string.
               Prefix can contain the following escape codes:
               - \@f Name of the file in which output call was made.
               - \@l Line number in the file in which output call was made.
               - \@p Name of the function from which output call was made.
               - \@r MPI rank of the calling process.  Will be empty if
                  MPI_COMM_WORLD size is 1.
               - \@R MPI rank of the calling process.  Will be 0 if
                  MPI_COMM_WORLD size is 1.
               - \@i Thread Id of the calling process.  Will be empty if
                  number of threads is 1.
               - \@I Thread Id of the calling process.  Will be 0 if
                  number of threads is 1.
               - \@x Rank information of the calling process.  Will be empty if
                  number of MPI ranks and number of threads are both 1
                  Same as [\@r:\@i]
               - \@X Rank information of the calling process.  Will be [0.0] if
                  number of MPI ranks and number of threads are both 1
                  Same as [\@R:\@I]
               - \@t Simulation time.  Will be the raw simulation cycle time
                  retrieved from the SST Core.
    */
    void setPrefix(const std::string& prefix);

    /** Returns object prefix */
    std::string getPrefix() const;

    /** Sets object verbose mask
        @param verbose_mask Bitmask of allowed message types for debug(),
               verbose() and fatal().  The Output object will only output the
               message if the set bits of the output_bits parameter
               are set in the verbose_mask of the object.  It uses this logic:
               if (~verbose_mask & output_bits == 0) then output is enabled.
    */
    void setVerboseMask(uint32_t verbose_mask);

    /** Returns object verbose mask */
    uint32_t getVerboseMask() const;

    /** Sets object verbose level
        @param verbose_level Debugging output level.  Calls to debug(),
               verbose() and fatal() are only output if their output_level
               parameter is less than or equal to the verbose_level currently
               set for the object
    */
    void setVerboseLevel(uint32_t verbose_level);

    /** Returns object verbose level */
    uint32_t getVerboseLevel() const;

    /** Sets object output location
        @param location Output location.  Output will be directed to STDOUT,
               STDERR, FILE, or NONE.  If FILE output is selected, the
               output will be directed to the file defined by the
               --debug runtime parameter, or to the file 'sst_output' if the
               --debug parameter is not defined.  If the size of MPI_COMM_WORLD
               is > 1, then the rank process will be appended to the file name.
    */
    void setOutputLocation(output_location_t location);

    /** Returns object output location */
    output_location_t getOutputLocation() const;

    /** This method allows for the manual flushing of the output. */
    inline void flush() const {std::fflush(*m_targetOutputRef);}


    /** This method sets the static filename used by SST.  It can only be called
        once, and is automatically called by the SST Core.  No components should
        call this method.
     */
    static void setFileName(const std::string& filename);

    static Output& getDefaultObject() { return m_defaultObject; }

private:
    // Support Methods
    void setTargetOutput(output_location_t location);
    void openSSTTargetFile() const;
    void closeSSTTargetFile();
    // std::string getMPIProcName() const;
    int getMPIWorldRank() const;
    int getMPIWorldSize() const;
    uint32_t getNumThreads() const;
    uint32_t getThreadRank() const;
    std::string buildPrefixString(uint32_t line,
                                  const std::string& file,
                                  const std::string& func) const;
    void outputprintf(uint32_t line,
                      const std::string &file,
                      const std::string &func,
                      const char *format,
                      va_list arg) const;
    void outputprintf(const char *format, va_list arg) const;

    friend int ::main(int argc, char **argv);
    static Output& setDefaultObject(const std::string& prefix, uint32_t verbose_level,
               uint32_t verbose_mask, output_location_t location,
               std::string localoutputfilename = "")
    {
        m_defaultObject.init(prefix, verbose_level, verbose_mask, location, localoutputfilename);
        return getDefaultObject();
    }

    static void setWorldSize(const RankInfo &ri, int mpiRank)
    {
        m_worldSize = ri;
        m_mpiRank = mpiRank;
    }

    static void setThreadID(std::thread::id mach, uint32_t user)
    {
        m_threadMap.insert(std::make_pair(mach, user));
    }


    // Internal Member Variables
    bool              m_objInitialized;
    std::string       m_outputPrefix;
    uint32_t          m_verboseLevel;
    uint32_t          m_verboseMask;
    output_location_t m_targetLoc;

    static Output     m_defaultObject;

    // m_targetOutputRef is a pointer to a FILE* object.  This is because
    // the actual FILE* object (m_sstFileHandle) is not created on construction,
    // but during the first call any of output(), verbose() or debug() methods.
    // m_targetOutputRef points to either &m_sstFileHandle, &stdout, or &stderr
    // depending upon constructor for the object.  However m_sstFileHandle is a
    // static variable that is set by the startup of SST, and the location
    // cannot be changed in the constructor or a call to setFileName().
    std::FILE**       m_targetOutputRef;

    // m_targetFileHandleRef, m_targetFileNameRef, and m_targetFileAccessCount
    // are pointers to their associated types.  These point to either the local 
    // output file information or to the global simulation output file information.
    std::FILE**        m_targetFileHandleRef;
    std::string*       m_targetFileNameRef;
    uint32_t*          m_targetFileAccessCountRef;

    // Static Member Variables regarding the Global simulation file info
    static std::string  m_sstGlobalSimFileName;
    static std::FILE*   m_sstGlobalSimFileHandle;
    static uint32_t     m_sstGlobalSimFileAccessCount;
    
    // File Member Variables regarding the local simulation file info
    std::string  m_sstLocalFileName;
    std::FILE*   m_sstLocalFileHandle;
    uint32_t     m_sstLocalFileAccessCount;

    static std::unordered_map<std::thread::id, uint32_t> m_threadMap;
    static RankInfo m_worldSize;
    static int m_mpiRank;
    
};

// Class to easily trace function enter and exit
class TraceFunction {
public:
    TraceFunction(uint32_t line, const char* file, const char* func);
    ~TraceFunction();

    Output& getOutput() {return output;}
    
private:
    Output output;
    uint32_t line;
    std::string file;
    std::string function;
    uint32_t rank;
    uint32_t thread;
};


} // namespace SST

#endif // SST_CORE_OUTPUT_H

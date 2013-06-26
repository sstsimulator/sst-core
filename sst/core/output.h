// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_OUTPUT_H
#define SST_OUTPUT_H

// UNCOMMENT OUT THIS LINE TO ENABLE THE DEBUG METHOD -OR_
// CHOOSE THE --enable-debug OPTION DURING SST CONFIGURATION
//#define __SST_DEBUG_OUTPUT__

#include <cstdio>

namespace SST {

// MACROS TO HELP BUILD THE CALLING FUNCTIONS INFORMATION
#define CALL_INFO      __LINE__, __FILE__, __FUNCTION__    

#if defined(__GNUC__) || defined(__clang__)
#define CALL_INFO_LONG __LINE__, __FILE__, __PRETTY_FUNCTION__
#else
#define CALL_INFO_LONG __LINE__, __FILE__, __FUNCTION__
#endif

/**
 * Output object provides consistant method for outputing data to 
 * stdout, stderr and/or sst debug file.  All components should 
 * use this class to log any information.
 */
class Output 
{
public:
    enum output_location_t { NONE, STDOUT, STDERR, FILE };
    
    /** Constructor.  Set up output configuration. 
        @param prefix Prefix to be prepended to all strings emitted by calls to 
               debug() or verbose().
               Prefix can contain the following escape codes:
               @f Name of the file in which output call was made.
               @l Line number in the file in which output call was made.
               @p Name of the function from which output call was made.
               @r MPI rank of the calling process.  Will be empty if 
                  MPI_COMM_WORLD size is 1. 
               @R MPI rank of the calling process.  Will be 0 if 
                  MPI_COMM_WORLD size is 1. 
        @param verbose_mask Bitmask of allowed message types for debug() and
               verbose().  The Output object will only output the message if all 
               set bits of the output_bits parameter of debug() and verbose() 
               are set in the verbose_mask of the object. 
        @param verbose_level Debugging output level.  Calls to debug() and 
               verbose() are only output if their output_level parameter is equal
               or greater than the verbose_level currently set for the object 
        @param location Output location.  Ouput will be directed to stdout, 
               stderr, to a file, or nowhere.  If FILE output is selected, the 
               output will be directed to the file defined by the 
               --debug runtime parameter, or to the file 'sst_output' if the 
               --debug parameter is not defined.  If the size of MPI_COMM_WORLD 
               is > 1, then the rank process will be appended to the file name.  
    */
    // CONSTRUCTION / DESTRUCTION
    Output(const std::string& prefix, uint32_t verbose_level,  
           uint32_t verbose_mask, output_location_t location);

    /** Default Constructor.  User must call init() to properly initialize obj.
        Until init() is called, no output will occur.
    */
    Output();  // Default constructor

    virtual ~Output();

    
    /** Initialize the object after construction 
        @param prefix Prefix to be prepended to all strings emitted by calls to 
               debug() or verbose().
               Prefix can contain the following escape codes:
               @f Name of the file in which output call was made.
               @l Line number in the file in which output call was made.
               @p Name of the function from which output call was made.
               @r MPI rank of the calling process.  Will be empty if 
                  MPI_COMM_WORLD size is 1. 
               @R MPI rank of the calling process.  Will be 0 if 
                  MPI_COMM_WORLD size is 1. 
        @param verbose_mask Bitmask of allowed message types for debug() and
               verbose().  The Output object will only output the message if all 
               set bits of the output_bits parameter of debug() and verbose() 
               are set in the verbose_mask of the object. 
        @param verbose_level Debugging output level.  Calls to debug() and 
               verbose() are only output if their output_level parameter is equal
               or greater than the verbose_level currently set for the object 
        @param location Output location.  Ouput will be directed to stdout, 
               stderr, to a file, or nowhere.  If FILE output is selected, the 
               output will be directed to the file defined by the 
               --debug runtime parameter, or to the file 'sst_output' if the 
               --debug parameter is not defined.  If the size of MPI_COMM_WORLD 
               is > 1, then the rank process will be appended to the file name.  
    */
    // INITIALIZATION
    void init(const std::string& prefix, uint32_t verbose_level,  
              uint32_t verbose_mask, output_location_t location);
    
    // OUTPUT METHODS
    // NOTE: __ATTRIBUTE__ Performs printf type mismatch checks on the format parameter
    /** Output the message with formatting as specified by the format parameter.
        @param format Format string.  All valid formats for printf are available.
        @param ... Argument strings for format.  
     */
    void output(const char* format, ...) 
         __attribute__ ((format (printf, 2, 3)));

    /** Output the message with formatting as specified by the format parameter.
        The output will be prepended with the expanded prefix set in the object.
        @param line Line number of calling function (use CALL_INFO macro)
        @param file File name calling function (use CALL_INFO macro)
        @param func Function name calling function (use CALL_INFO macro)
        @param format Format string.  All valid formats for printf are available.
        @param ... Argument strings for format.  
     */
    void output(uint32_t line, std::string file, std::string func, const char* format, ...) 
         __attribute__ ((format (printf, 5, 6)));
       
    /** Output the verbose message with formatting as specified by the format 
        parameter. Output will only occur if specified output_level and 
        output_bits meet criteria defined by object.  The output will be 
        prepended with the expanded prefix set in the object.
        @param line Line number of calling function (use CALL_INFO macro)
        @param file File name calling function (use CALL_INFO macro)
        @param func Function name calling function (use CALL_INFO macro)
        @param output_level For output to occur, output_level must be greater
               than verbose_level set in object
        @param output_bits For output to occur, all bits in output_bits must 
               be set the in the verbose_mask set in object
        @param format Format string.  All valid formats for printf are available.
        @param ... Argument strings for format.  
     */
    void verbose(uint32_t line, std::string file, std::string func, uint32_t output_level, 
                  uint32_t output_bits, const char* format, ...)    
                  __attribute__ ((format (printf, 7, 8)));
    
    /** Output the debug message with formatting as specified by the format 
        parameter. Output will only occur if specified output_level and 
        output_bits meet criteria defined by object.  The output will be 
        prepended with the expanded prefix set in the object.
        NOTE: Debug ouputs will only occur if the __SST_DEBUG_OUTPUT__ is defined.
              this define can be set in source code or by setting the
              --enable-debug option during SST configuration.
        @param line Line number of calling function (use CALL_INFO macro)
        @param file File name calling function (use CALL_INFO macro)
        @param func Function name calling function (use CALL_INFO macro)
        @param output_level For output to occur, output_level must be greater
               than verbose_level set in object
        @param output_bits For output to occur, all bits in output_bits must 
               be set the in the verbose_mask set in object
        @param format Format string.  All valid formats for printf are available.
        @param ... Argument strings for format.  
     */
    void debug(uint32_t line, std::string file, std::string func, uint32_t output_level, 
               uint32_t output_bits, const char* format, ...)   
               __attribute__ ((format (printf, 7, 8)));
    
    // GET / SET METHODS
    
    /** Sets object prefix
        @param prefix Prefix to be prepended to all strings emitted by calls to 
               debug() or verbose().
               Prefix can contain the following escape codes:
               @f Name of the file in which output call was made.
               @l Line number in the file in which output call was made.
               @p Name of the function from which output call was made.
               @r MPI rank of the calling process.  Will be empty if 
                  MPI_COMM_WORLD size is 1. 
               @R MPI rank of the calling process.  Will be 0 if 
                  MPI_COMM_WORLD size is 1. 
    */
    void setPrefix(const std::string& prefix);

    /** Returns object prefix */
    std::string getPrefix() const;

    /** Sets object verbose mask
        @param verbose_mask Bitmask of allowed message types for debug() and
               verbose().  The Output object will only output the message if all 
               set bits of the output_bits parameter of debug() and verbose() 
               are set in the verbose_mask of the object. 
    */
    void setVerboseMask(uint32_t verbose_mask);

    /** Returns object verbose mask */
    uint32_t getVerboseMask() const;

    /** Sets object verbose level
        @param verbose_level Debugging output level.  Calls to debug() and 
               verbose() are only output if their output_level parameter is equal
               or greater than the verbose_level currently set for the object 
    */
    void setVerboseLevel(uint32_t verbose_level);

    /** Returns object verbose level */
    uint32_t getVerboseLevel() const;
    
    /** Sets object output location
        @param location Output location.  Ouput will be directed to stdout, 
               stderr, to a file, or nowhere.  If FILE output is selected, the 
               output will be directed to the file defined by the 
               --debug runtime parameter, or to the file 'sst_output' if the 
               --debug parameter is not defined.  If the size of MPI_COMM_WORLD 
               is > 1, then the rank process will be appended to the file name.  
    */
    void setOutputLocation(output_location_t location);

    /** Returns object output location */
    output_location_t getOutputLocation() const;
    
               
    /** This method sets the static filename used by SST.  It can only be called
        once, and is automatically called by the SST Core.  No components should
        call this method.
     */
    static void setFileName(const std::string& filename);

private:
    // Support Methods
    void setTargetOutput(output_location_t location);
    void openSSTTargetFile();
    void closeSSTTargetFile();
    std::string buildPrefixString(uint32_t line, std::string& file, std::string& func);
    void outputprintf (std::string& prefixStr, const char* format, va_list arg);   

    // Internal Member Variables
    bool              m_objInitialized;
    std::string       m_outputPrefix;
    uint32_t          m_verboseLevel;
    uint32_t          m_verboseMask;
    output_location_t m_targetLoc;    
    int               m_MPIWorldSize;
    int               m_MPIWorldRank;
    std::string       m_MPIProcName;
    
    // m_targetOutputRef is a pointer to a FILE* object.  This is because
    // the actual FILE* object (m_sstFileHandle) is not created on construction,
    // but during the first call any of output(), verbose() or debug() methods.  
    // m_targetOutputRef points to either &m_sstFileHandle, &stdout, or &stderr 
    // depending upon constructor for the object.  However m_sstFileHandle is a 
    // static variable that is set by the startup of SST, and the location
    // cannot be changed in the constructor or a call to setFileName().
    std::FILE**       m_targetOutputRef;  

    // Static Member Variables
    static std::string m_sstFileName;
    static std::FILE*  m_sstFileHandle;
    static uint32_t    m_sstFileAccessCount;
  
    // Serialization Methods
    friend class boost::serialization::access;
    template<class Archive> void serialize(Archive & ar, const unsigned int version);
};

} // namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Output)

#endif // SST_OUTPUT_H

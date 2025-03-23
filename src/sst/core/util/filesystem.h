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

#ifndef SST_CORE_FILESYSTEM_H
#define SST_CORE_FILESYSTEM_H

#include <cstdio>
#include <iostream>
#include <mutex>
#include <stdexcept>

// REMOVE ME
#include <filesystem>

namespace SST {
namespace Util {


/**
   Class used to manage files and directories. One of the features is
   that you can set the base directory to be used for all files and
   directories sepcified with a relative path.
 */
class Filesystem
{
public:
    Filesystem() = default;

    /**
       Set the base path to be use when relative paths are used. If a
       relative path is passed into this function, then the current
       working directory will be used as the base path.  When the
       base_path is set, the class will check to see if the path
       already exists, and if not, whether or not the user has
       permissions to create the directory. If the directory doesn't
       exist and the user doesn't have permission to create it, the
       runction will return false and thee base_path will remain
       unchanged.  If the directory doesn't exist and the user has
       permissions to create it, the directory will be created on
       demand.

       @param base_path Path to set as the base path. The path can be
       absolute or relative, and a path starting with ~/ with the ~
       expanded to the user's home directory.

       @return True if path is valid and either exists and is writable
       by the user or can be created, false otherwise.
    */
    bool setBasePath(const std::string& base_path);

    /**
       This function will ensure a directory exists, and if it doesn't
       already exist, it will create it.  If the directory starts with
       / or ~/ (where ~ represents your home directory) then the path
       will be considered absolute.  Otherwise, the path will be
       relative to base path set using setBasePath(), or the current
       working directory if a base path has not been set, or the
       filename begins with ./ or ../. All intermediate directories
       that don't exist will be created.

       @param path Path to verify or create

       @param strip_filename Set to true if input is a file and you
       need to strip the filename to get the directory name. Default
       is false.

       @return false if directory exists but is not writable, true
       otherwise

       @throws std::filesystem::filesystem_error Thrown when the
       directory does not exist and the function tries to create it
       but sees an underlying OS API error (see
       std::filesystem::create_directories()); the most likely cause
       is a permissions error, or an identically named file.

       @throws std::invalid_argument Thrown when the passed in path
       starts with ~/ and the home directory cannot be determined.
    */
    bool ensureDirectoryExists(std::filesystem::path p, bool strip_filename = false);


    /**
       Creates a unique directory.  If the specified directory already
       exists, the function will append _1 to the directory name.  If
       that is also taken, it will increment the ID until it finds the
       lowest index that is not used.  The function will also create
       any directory in the path that does not already exist, but only
       the final directory will be made unique.

       @param dir_name Name of directory to create.  Directory can be
       relative or absolute and all intermediate paths that don't
       exist will be created. The unique name only applies to the last
       level directory.

       @return Absolute path of the created directory

       @throws std::filesystem::filesystem_error Thrown when the call
       sees an underlying OS API error (see
       std::filesystem::create_directories()); the most likely cause
       is a permissions error, or an file named identically to one of
       the directory names that needs to be created.

       @throws std::invalid_argument Thrown when the passed in path
       starts with ~/ and the home directory cannot be determined.

    */
    std::string createUniqueDirectory(std::filesystem::path dir_name);


    /**
       Open a file using std::fopen().  If the passed-in path is
       relative, it will be relative to the path set using
       setBasePath().  If setBasePath() was not called, then it will
       be relative to the current working directory.  The FILE*
       returned from this function can be closed by using
       std::fclose().

       @param filename Name of file to open.

       @param mode read/write mode as defined by std::fopen()

       @return FILE* to opened file
    */
    FILE* fopen(const std::string& filename, const char* mode);


    /**
       Get the absolute path for a directory or name. If the directory
       starts with / or ~/ (where ~ represents your home directory)
       then the path will be considered absolute.  Otherwise, the path
       will be relative to the base_path, or the current working
       directory if no base_path was set or the path starts with ./ or
       ../

       @param path Path to turn into an absolute path, based on the
       base_path of the filesystem object

       @return returns the absolute path

       @throws std::invalid_argument Thrown when base_path is not
       absolute (in format, there is no check to see if the directory
       actually exists) and the passed in path is relative.  Also
       thrown when the passed in path starts with ~/ and the home
       directory cannot be determined.
    */
    std::string getAbsolutePath(const std::string& path);

    /**
       Open a file using ofstream.  If the passed-in path is relative,
       it will be relative to the path set using setBasePath().  If
       setBasePath() was not called, then it will be relative to the
       current working directory.

       @param filename Name of file to open

       @param mode read/write mode as defined by std::ofstream

       @return ofstream object representing the new opened file
     */
    std::ofstream ofstream(const std::string& filename, std::ios_base::openmode mode = std::ios_base::out);


    /**
       Get the base_path set by setBasePath

       @return base_path set by setBasePath.  This will be an absolute
       path
     */
    std::string getBasePath() const { return base_path_.string(); }


    /**
       Get the absolute path for a directory or name. If the directory
       starts with / or ~/ (where ~ represents your home directory)
       then the path will be considered absolute.  Otherwise, the path
       will be relative to the set base_path, or the current working
       directory if no base_path was set, or the path starts with ./
       or ../

       @param path Path to turn into an absolute path

       @param base_path Path to use as the base path for relative
       paths passed into the path parameter.  If an empty string is
       passed in, then the current working directory will be used as
       the base_path (default behavior if you don't pass in a
       base_path).  base_path must be an absolute path and cannot
       include a ~

       @throws std::invalid_argument Thrown when base_path is not
       absolute (in format, there is no check to see if the directory
       actually exists) and the passed in path is relative.  Also
       thrown when the passed in path starts with ~/ and the home
       directory cannot be determined.
    */
    static std::string getAbsolutePath(const std::string& path, std::string base_path);

    /**
       Gets a random file name for use as a temporary file or
       directory
     */
    static std::string getRandomName(size_t length = 8);

private:
    std::filesystem::path base_path_;
    static std::mutex     create_mutex_;
};

void test_filesystem();

} // end namespace Util
} // end namespace SST

#endif // SST_CORE_FILESYSTEM_H

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

#include "filesystem.h"

#include <chrono>
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <random>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace SST::Util {

std::mutex Filesystem::create_mutex_;

// Normalize the path (remove redundant separators and up-level references)
// void normalize() {
//     std::vector<std::string> parts;
//     std::string part;
//     size_t start = 0;

//     while ((start = path_.find_first_not_of("/\\", start)) != std::string::npos) {
//         size_t end = path_.find_first_of("/\\", start);
//         part = path_.substr(start, end - start);
//         if (part == "..") {
//             if (!parts.empty()) {
//                 parts.pop_back(); // Go up one directory
//             }
//         } else if (part != ".") {
//             parts.push_back(part);
//         }
//         start = end;
//     }

//     path_ = "/" + join(parts);
// }


/****** Utility functions for use in the file ********/

/**
   Check to see if directory is writeable. If the directory doesn't
   exist, false will be returned. Path must be an absolute path and if
   it is not, function will return false.

   @param path path to check

   @return true if path is writable, false otherwise
 */
bool
isDirectoryWritable(const std::string& path)
{

    fs::path test_file_path = path;
    if ( !test_file_path.is_absolute() ) {
        return false;
    }

    if ( !fs::is_directory(path) ) {
        return false;
    }

    // Check to see if it's writable by opening a randomly named file
    std::string test_file_name = Filesystem::getRandomName(16) + ".tmp";

    test_file_path /= test_file_name;

    // Attempt to create and write to the file
    std::ofstream test_file(test_file_path.string());
    if ( test_file ) {
        test_file << "This is a test file." << std::endl; // Write something to the file
        test_file.close();                                // Close the file
        fs::remove(test_file_path);                       // Clean up by removing the file

        return true; // Directory is writable
    }
    else {
        return false; // Directory is not writable
    }
}


/****** Class functions for Filesystem ********/

bool
Filesystem::setBasePath(const std::string& base_path)
{
    // Check to see if base_path exists.  If not, see if we will be
    // able to create it when the time comes.
    std::string absolute_path = getAbsolutePath(base_path, "");

    if ( fs::is_directory(absolute_path) ) {
        if ( isDirectoryWritable(absolute_path) ) {
            base_path_ = absolute_path;
            return true;
        }
        else {
            return false;
        }
    }
    else if ( fs::exists(absolute_path) ) {
        // This means there is a file with the same name
        return false;
    }

    fs::path path = absolute_path;

    // Directory doesn't exist.  See if we can create it.  We do this
    // by marching back up the path until we find a path that exists,
    // then see if we can create a directory in it.  If so, we assume
    // we can create the whole path.
    fs::path parent = path.parent_path();
    while ( !fs::is_directory(parent) ) {
        parent = parent.parent_path();
    }

    // Check to see if this existing directory is writeable
    if ( isDirectoryWritable(parent.string()) ) {
        base_path_ = absolute_path;
        return true;
    }

    return false;
}


bool
Filesystem::ensureDirectoryExists(std::filesystem::path p, bool strip_filename)
{
    // Can only be in one function that may create a directory at a time
    std::lock_guard<std::mutex> guard(create_mutex_);

    // If strip_filename is true, then this is a filename and we need
    // to strip it off to get the path
    if ( strip_filename ) p = p.parent_path();

    // We assume this is a directory, so if there is a trailing / we
    // need to remove it
    std::string absolute_path = getAbsolutePath(p.string(), base_path_.string());
    if ( absolute_path.back() == '/' ) absolute_path.pop_back();

    fs::path path = absolute_path;

    if ( fs::is_directory(path) ) {
        return isDirectoryWritable(absolute_path);
    }

    return fs::create_directories(path);
}

std::string
Filesystem::createUniqueDirectory(std::filesystem::path dir_name)
{
    std::string absolute_path = getAbsolutePath(dir_name, base_path_.string());
    if ( absolute_path.back() == '/' ) absolute_path.pop_back();

    // Make sure that the base directory exists
    ensureDirectoryExists(absolute_path, true);

    fs::path path(absolute_path);

    if ( fs::exists(path) ) {
        // Append an _ and a unique number to the directory name.
        // Start at _1 and keep adding 1 until unique name is found
        int      num = 0;
        fs::path p2;
        do {
            ++num;
            p2 = path;
            p2 += fs::path(std::string("_") + std::to_string(num));
        } while ( exists(p2) ); // Ensure the new directory name is unique

        path += fs::path(std::string("_") + std::to_string(num));
    }

    // This will throw an exception if the path is not writeable
    fs::create_directories(path);

    return path.string();
}

std::string
Filesystem::getAbsolutePath(const std::string& path)
{
    // If this is called, we need to make sure that all the
    // directories are created.
    std::string p = getAbsolutePath(path, base_path_);
    ensureDirectoryExists(p, true);
    return p;
}

FILE*
Filesystem::fopen(const std::string& filename, const char* mode)
{
    std::string absolute_path = getAbsolutePath(filename, base_path_.string());
    ensureDirectoryExists(absolute_path, true);
    return std::fopen(absolute_path.c_str(), mode);
}

std::ofstream
Filesystem::ofstream(const std::string& filename, std::ios_base::openmode mode)
{
    std::string absolute_path = getAbsolutePath(filename, base_path_.string());
    ensureDirectoryExists(absolute_path, true);
    return std::ofstream(absolute_path.c_str(), mode);
}


/******** Static functions for Filesystem ********/

std::string
Filesystem::getAbsolutePath(const std::string& path, std::string base_path)
{
    fs::path ap(path);

    // Check if the directory is absolute
    if ( ap.string().find("~/") == 0 ) {
        // Expand the home directory
        const char* home = getenv("HOME");
        if ( home ) {
            ap = fs::path(home) / ap.string().substr(2);
        }
        else {
            throw std::invalid_argument(
                "path starting with ~/ passed to getAbsolutePath(), but home directory could not be determined");
        }
    }
    else if ( !ap.is_absolute() ) {
        // If it's not absolute, make it relative to the base_path or
        // current working directory
        if ( base_path != "" ) {
            // Check to make sure base_path is absolute
            fs::path base(base_path);
            if ( !base.is_absolute() ) {
                std::string err_msg("Passed in base_path was not absolute: ");
                err_msg += base_path;
                throw std::invalid_argument(err_msg);
            }
            ap = fs::path(base_path) / ap;
        }
        else {
            ap = fs::current_path() / ap;
        }
    }
    return ap.string();
}

std::string
Filesystem::getRandomName(size_t length)
{
    // Get the current time in nanoseconds to seed the random number generator
    auto                                         now  = std::chrono::high_resolution_clock::now();
    auto                                         seed = now.time_since_epoch().count();
    std::mt19937                                 generator(seed);
    std::uniform_int_distribution<unsigned char> distribution('a', 'z');

    // Generate a random file name
    std::string random_name = "temp_";
    for ( size_t i = 0; i < length; ++i ) {
        random_name += static_cast<char>(distribution(generator));
    }

    return random_name;
}

} // namespace SST::Util

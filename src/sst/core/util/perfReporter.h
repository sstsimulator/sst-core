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

#ifndef SST_CORE_PERF_REPORTER_H
#define SST_CORE_PERF_REPORTER_H

#include "sst/core/output.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/util/filesystem.h"

#include "nlohmann/json.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <stack>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace SST::Util {

class DataRecord;
class PerfReporter;

class PerfData
{
    friend DataRecord;

public:
    // Create (begin) a new record level
    PerfData(std::string name, PerfData* parent);

    template <typename T>
    T getData(const std::string& key)
    {
        const auto& val = data_.at(key);
        return std::get<T>(val);
    }

    std::string                                                                              name_ = "";
    std::map<std::string, std::variant<uint64_t, int64_t, double, UnitAlgebra, std::string>> data_;
    std::map<std::string, std::pair<std::string, std::string>>
                           keys_; // Keys optionally map to a <extended_key, unit> pair for pretty print
    PerfData*              parent_ = nullptr;
    std::vector<PerfData*> children_;
};

// Stores a hierarchy of named key/val pairs that should be output
class DataRecord
{
public:
    friend PerfReporter;
    enum class TextFormat { plain, tree, list };
    DataRecord(std::string name, TextFormat format);

    // API for inserting data into records
    // Map of key str to pair of (pretty print string, units). Units may be "" if not applicable.
    // Keys are *not* required to be entered in this structure if no pretty print string/units are required
    // Extended string for the name of the record can also be entered
    void setKeys(std::map<std::string, std::pair<std::string, std::string>>& key_map);

    void setFormat(TextFormat format) { format_ = format; }

    // API for traversing
    void changeLevelUp();
    bool changeLevelDown(std::string name); // Return whether level was changed
    void addChild(std::string name);
    void endChild();
    void addData(std::string key, double value);
    void addData(std::string key, uint64_t value);
    void addData(std::string key, int64_t value);
    void addData(std::string key, UnitAlgebra value);
    void addData(std::string key, std::string value);
    void addData(std::map<std::string, std::variant<uint64_t, int64_t, double, UnitAlgebra, std::string>> data);

private:
    PerfData*  root_;
    PerfData*  current_record_;
    TextFormat format_;
};

class PerfReporter
{
public:
    PerfReporter() { output_.init("", 0, 0, Output::STDOUT); }

    /*
     * Thread-safe function to return the data record with the given name
     * If record does not already exist, it will be created
     */
    DataRecord* createDataRecord(std::string name, DataRecord::TextFormat format = DataRecord::TextFormat::plain);

    void output(int rank, int num_ranks); // Output all registered records

    void configureOutput(std::string output_str);
    void outputRecordToTextTree(const PerfData* node, std::stringstream* sstr);

    void outputNodeToTextTree(
        const PerfData* node, std::stringstream* sstr, bool last_child, std::vector<int> levels_has_more_siblings = {});
    void printTreeIndent(
        std::stringstream* sstr, const std::vector<int>& levels_has_more_siblings, size_t indent_level);
    void outputValueToText(
        const std::variant<uint64_t, int64_t, double, UnitAlgebra, std::string>& v, std::stringstream* sstr);
    std::string convertValueToString(const std::variant<uint64_t, int64_t, double, UnitAlgebra, std::string>& v);
    void   outputRecordToText(const PerfData* node, std::stringstream* sstr, int indent = 0, bool print_name = true);
    void   outputRecordToTextList(const PerfData* node, std::stringstream* sstr, bool header);
    void   outputRecordToJSON(const PerfData* node, nlohmann::ordered_json* obj);
    size_t recordCount();

private:
    // Can output to console and/or a file (json or text)
    Output      output_;
    bool        output_console_ = false; // output to console or not
    std::string filename_;               // File to output to (or "" if console only; extension determines format)
    std::map<std::string, DataRecord*>
                       records_; // Data stored at the base level - no indent (txt) or highest-level object (JSON)
    mutable std::mutex mtx;      // mutex for record creation
};

} // namespace SST::Util

#endif // SST_CORE_PERF_REPORTER_H

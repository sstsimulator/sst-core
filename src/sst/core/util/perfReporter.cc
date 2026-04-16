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
//

#include "sst_config.h"

#include "sst/core/util/perfReporter.h"

#include "sst/core/cputimer.h"
#include "sst/core/memuse.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/sst_mpi.h"

#include "nlohmann/json.hpp"

#include <clocale>
#include <cstdio>

namespace json = ::nlohmann;

namespace SST::Util {

PerfData::PerfData(std::string name, PerfData* parent) :
    name_(name),
    parent_(parent)
{
    if ( parent_ ) {
        parent_->children_.push_back(this);
    }
}

// PerfData
DataRecord::DataRecord(std::string name, DataRecord::TextFormat format)
{
    root_           = new PerfData(name, nullptr);
    current_record_ = root_;
    format_         = format;
}

void
DataRecord::setKeys(std::map<std::string, std::pair<std::string, std::string>>& key_map)
{
    // Insert w/ overwrite if duplicate keys
    for ( auto const& entry : key_map ) {
        current_record_->keys_.insert_or_assign(entry.first, entry.second);
    }
}

void
DataRecord::changeLevelUp()
{
    if ( current_record_ == root_ ) return;
    current_record_ = current_record_->parent_;
}

bool
DataRecord::changeLevelDown(std::string name)
{
    // No lookup so linear search children for matching name
    for ( auto const& child : current_record_->children_ ) {
        if ( child->name_ == name ) {
            current_record_ = child;
            return true;
        }
    }
    return false; // Not found
}

void
DataRecord::addChild(std::string name)
{
    PerfData* child = new PerfData(name, current_record_);
    current_record_ = child;
}

void
DataRecord::endChild()
{
    if ( current_record_ == root_ ) return;
    current_record_ = current_record_->parent_;
}

void
DataRecord::addData(std::string key, double value)
{
    current_record_->data_[key] = value;
}

void
DataRecord::addData(std::string key, uint64_t value)
{
    current_record_->data_[key] = value;
}

void
DataRecord::addData(std::string key, int64_t value)
{
    current_record_->data_[key] = value;
}

void
DataRecord::addData(std::string key, std::string value)
{
    current_record_->data_[key] = value;
}
void
DataRecord::addData(std::string key, UnitAlgebra value)
{
    current_record_->data_[key] = value;
}

void
DataRecord::addData(std::map<std::string, std::variant<uint64_t, int64_t, double, UnitAlgebra, std::string>> data)
{
    for ( auto const& entry : data ) {
        current_record_->data_.insert_or_assign(entry.first, entry.second);
    }
}

DataRecord*
PerfReporter::createDataRecord(std::string name, DataRecord::TextFormat format)
{
    std::lock_guard<std::mutex> lock(mtx);
    DataRecord*                 record;
    auto                        record_it = records_.find(name);
    if ( record_it == records_.end() ) {
        record = new DataRecord(name, format);
        records_.insert(std::make_pair(name, record));
    }
    else {
        record = record_it->second;
    }
    return record;
}

void
PerfReporter::configureOutput(std::string output_str)
{
    auto comma_pos = output_str.find_first_of(',');
    if ( comma_pos != std::string::npos ) {
        if ( output_str.substr(0, comma_pos) == "stdout" ) {
            output_console_ = true;
            filename_       = output_str.substr(comma_pos + 1);
        }
        else if ( output_str.substr(comma_pos + 1) == "stdout" ) {
            output_console_ = true;
            filename_       = output_str.substr(0, comma_pos);
        }
        else {
            // Error, bad string
            output_.fatal(CALL_INFO, -1,
                "Error: Bad arguments to '--profiling-output'. Should be 'stdout', 'filename', or 'stdout,filename' "
                "where filename has an extension of '.txt' or .json'. Got '%s'\n",
                output_str.c_str());
        }
    }
    else if ( output_str == "stdout" ) {
        output_console_ = true;
    }
    else {
        filename_ = output_str;
    }

    if ( filename_ != "" ) {
        size_t      pos = filename_.find_last_of('.');
        std::string ext = filename_.substr(pos + 1);
        if ( ext == "json" || ext == "txt" ) {
            if ( !Simulation_impl::filesystem.ensureDirectoryExists(filename_, true) ) {
                output_.fatal(CALL_INFO, -1,
                    "Error: Unable to write a file to the directory for profiling output (filename='%s'). Check that "
                    "this location is writeable.",
                    filename_.c_str());
            }
        }
        else {
            output_.fatal(CALL_INFO, -1,
                "Error: Profiling output can only be written in text or json format; specify a filename ending in "
                "'.txt' or '.json'. Got '%s'.\n",
                filename_.c_str());
        }
    }
}

/*
 * Output is coordinated by rank 0 which will request data from
 * each other rank
 *
 * For each record name
 *  - Rank 0 broadcasts record name
 *  - Ranks serially output the data (or send data to rank 0 one by one)
 *
 */
void
PerfReporter::output(int rank, int num_ranks)
{
    bool output_txt  = false;
    bool output_json = false;

    // A helper for rank exchanges
    auto recvStringFromRank = [](int src) -> std::string {
        int len = 0;
        SST_MPI_Recv(&len, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if ( len <= 0 ) return {};
        std::string s(static_cast<size_t>(len), '\0');
        SST_MPI_Recv(s.data(), len, MPI_CHAR, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        return s;
    };

    // Broadcast number of records from rank 0 to all others
    uint32_t record_count = 0;
    if ( rank == 0 ) {
        record_count = static_cast<uint32_t>(records_.size());
    }
    SST_MPI_Bcast(&record_count, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);

    if ( record_count == 0 ) return; // Nothing to print

    // Set up files
    std::ofstream filestream;
    if ( !filename_.empty() ) {
        const auto        pos = filename_.find_last_of('.');
        const std::string ext = (pos == std::string::npos) ? "" : filename_.substr(pos + 1);
        output_txt            = (ext == "txt");
        output_json           = (ext == "json");
        filestream            = Simulation_impl::filesystem.ofstream(filename_);
    }

    std::cout << std::flush;

    if ( output_json ) {
        filestream << "{\n";
    }

    // MPI Rank 0 runs the controlling for loop
    if ( rank == 0 ) {
        uint32_t idx = 0;
        for ( auto& record : records_ ) {
            ++idx;

            // Broadcast the record currently being output
            int length = static_cast<int>(record.first.size());
            SST_MPI_Bcast(&length, 1, MPI_INT, 0, MPI_COMM_WORLD);
            SST_MPI_Bcast((void*)&record.first[0], length, MPI_CHAR, 0, MPI_COMM_WORLD);
            auto json_o = nlohmann::ordered_json::object();

            // Output any text formatted data, first for rank 0 and then append from other ranks
            if ( output_console_ || output_txt ) {
                std::stringstream str;
                if ( record.second->format_ == DataRecord::TextFormat::plain ) {
                    outputRecordToText(record.second->root_, &str);
                }
                else if ( record.second->format_ == DataRecord::TextFormat::tree ) {
                    outputRecordToTextTree(record.second->root_, &str);
                }
                else { // DataRecord::TextFormat::list
                    outputRecordToTextList(record.second->root_, &str, true);
                }

                const std::string rec_str = str.str();
                if ( output_console_ ) std::cout << "\n" << rec_str;
                if ( output_txt ) filestream << "\n" << rec_str;

                for ( int i = 1; i < num_ranks; i++ ) {
                    // Receive from each other rank
                    const std::string remote_str = recvStringFromRank(i);
                    if ( remote_str.empty() ) continue;
                    if ( output_console_ ) std::cout << remote_str;
                    if ( output_txt ) filestream << remote_str;
                }
            }

            // Output any JSON formatted data, first for rank 0 and then append from other ranks
            if ( output_json ) {
                filestream << '"' << record.second->root_->name_ << "\": {\n";
                outputRecordToJSON(record.second->root_, &json_o);
                size_t item_count = 0;
                for ( auto& [key, val] : json_o.items() ) {
                    item_count++;
                    filestream << "\"" << key << "\": " << val.dump(2);
                    if ( item_count != json_o.size() ) {
                        filestream << ",\n";
                    }
                }

                for ( int i = 1; i < num_ranks; i++ ) {
                    // Receive from each other rank
                    const std::string remote_str = recvStringFromRank(i);
                    if ( remote_str.empty() ) continue;
                    filestream << ",\n" << remote_str;
                }

                if ( idx != records_.size() ) {
                    filestream << "\n},\n";
                }
                else {
                    filestream << "\n}\n";
                }
            }
        }

        // Complete output and close files
        if ( output_json ) {
            filestream << "}\n";
        }

        if ( filestream ) {
            filestream.flush();
            filestream.close();
        }
    }
    else { // Not rank 0
        // All other ranks generate output strings for the currently-parsing record
        for ( uint32_t count = 0; count < record_count; count++ ) {
            // Get record name
            int length = 0;
            SST_MPI_Bcast(&length, 1, MPI_INT, 0, MPI_COMM_WORLD);
            char* name = new char[length];
            SST_MPI_Bcast(name, length, MPI_CHAR, 0, MPI_COMM_WORLD);

            // Lookup record
            auto record = records_.find(name);

            if ( output_console_ || output_txt ) {
                if ( record == records_.end() ) { // None found
                    int length = 0;
                    SST_MPI_Send(&length, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                }
                else {
                    std::stringstream str;
                    if ( record->second->format_ == DataRecord::TextFormat::plain ) {
                        outputRecordToText(record->second->root_, &str, 0, false);
                    }
                    else if ( record->second->format_ == DataRecord::TextFormat::tree ) {
                        outputRecordToTextTree(record->second->root_, &str);
                    }
                    else { // DataRecord::TextFormat::list
                        outputRecordToTextList(record->second->root_, &str, false);
                    }

                    std::string send_str = str.str();
                    int         length   = send_str.size();
                    SST_MPI_Send(&length, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                    SST_MPI_Send(send_str.data(), length, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
                }
            }

            if ( output_json ) {
                if ( record == records_.end() ) {
                    int length = 0;
                    SST_MPI_Send(&length, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                }
                else {
                    std::stringstream str;
                    auto              json_o = nlohmann::ordered_json::object();
                    outputRecordToJSON(record->second->root_, &json_o);
                    std::string send_str;
                    size_t      item_count = 0;
                    for ( auto& [key, val] : json_o.items() ) {
                        item_count++;
                        send_str += "\"" + key + "\": " + val.dump(2);
                        if ( item_count != json_o.size() ) {
                            send_str += ",";
                        }
                        send_str += "\n";
                    }

                    int length = send_str.size();
                    SST_MPI_Send(&length, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                    SST_MPI_Send(send_str.data(), length, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
                }
            }
            delete[] name;
        }
    }
}

void
PerfReporter::outputValueToText(
    const std::variant<uint64_t, int64_t, double, UnitAlgebra, std::string>& v, std::stringstream* sstr)
{
    std::visit(
        [&sstr](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr ( std::is_same_v<T, UnitAlgebra> ) {
                (*sstr) << arg.toStringBestSI();
            }
            else {
                (*sstr) << arg;
            }
        },
        v);
}

std::string
PerfReporter::convertValueToString(const std::variant<uint64_t, int64_t, double, UnitAlgebra, std::string>& val)
{
    return std::visit(
        [](auto&& arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;
            if constexpr ( std::is_same_v<T, UnitAlgebra> ) {
                return arg.toStringBestSI();
            }
            else if constexpr ( std::is_same_v<T, std::string> ) {
                return arg;
            }
            else {
                return std::to_string(arg);
            }
        },
        val);
}

void
PerfReporter::outputRecordToText(const PerfData* node, std::stringstream* sstr, int indent, bool print_name)
{
    if ( !node ) return;

    std::string indent_str(indent * 2, ' ');

    if ( print_name && node->parent_ == nullptr ) (*sstr) << "\n";

    // Print name, replacing if needed
    if ( print_name && node->keys_.find(node->name_) != node->keys_.end() ) {
        (*sstr) << indent_str << node->keys_.find(node->name_)->second.first << ":\n";
    }
    else if ( print_name ) {
        (*sstr) << indent_str << node->name_ << ":\n";
    }

    size_t max_key_width = 0;

    // Temp map so we can detect vertical alignment location
    std::vector<
        std::tuple<std::string, const std::variant<uint64_t, int64_t, double, UnitAlgebra, std::string>*, std::string>>
        entries;

    for ( const auto& [key, value] : node->data_ ) {
        std::string print_key = key;
        std::string units     = "";
        auto        it        = node->keys_.find(key);

        if ( it != node->keys_.end() ) {
            print_key = it->second.first;
            units     = it->second.second;
        }
        print_key += ":";

        if ( print_key.size() > max_key_width ) max_key_width = print_key.size();
        entries.emplace_back(print_key, &value, units);
    }

    for ( const auto& [print_key, value_ptr, units_str] : entries ) {
        (*sstr) << indent_str << "  " << std::left << std::setw(static_cast<int>(max_key_width)) << print_key << " ";
        outputValueToText(*value_ptr, sstr);
        if ( units_str != "" ) {
            (*sstr) << " " << units_str;
        }
        (*sstr) << "\n";
    }

    for ( const auto* child : node->children_ ) {
        outputRecordToText(child, sstr, indent + 1);
    }
}

void
PerfReporter::printTreeIndent(std::stringstream* sstr, const std::vector<int>& child_counts, size_t indent_level)
{
    for ( size_t i = 0; i < indent_level; ++i ) {
        if ( child_counts[i] > 0 ) {
            (*sstr) << "│ ";
        }
        else {
            (*sstr) << "  ";
        }
    }
}

void
PerfReporter::outputRecordToTextTree(const PerfData* node, std::stringstream* sstr)
{
    // Print metadata node
    if ( node->keys_.find(node->name_) != node->keys_.end() ) {
        (*sstr) << "\n" << node->keys_.find(node->name_)->second.first << ":\n";
    }
    else {
        (*sstr) << "\n" << node->name_ << ":\n";
    }

    // Print tree nodes
    if ( node->children_.empty() ) return;
    outputNodeToTextTree(node->children_[0], sstr, true);
}

void
PerfReporter::outputNodeToTextTree(
    const PerfData* node, std::stringstream* sstr, bool last_child, std::vector<int> level_info)
{
    if ( !node ) return;

    size_t level = level_info.size();

    // Print node name with appropriate markers
    if ( level == 0 ) {
        (*sstr) << "■ ";
    }
    else {
        // For non-root node, add the indent as needed
        printTreeIndent(sstr, level_info, level - 1);
        (*sstr) << (last_child ? "└ ■ " : "├ ■ ");
    }

    auto it_name = node->keys_.find(node->name_);
    if ( it_name != node->keys_.end() && !it_name->second.first.empty() ) {
        (*sstr) << it_name->second.first << "\n";
    }
    else {
        (*sstr) << node->name_ << "\n";
    }

    // Print key/vals for this node
    std::vector<std::string> keys_found;
    for ( const auto& [key, val] : node->data_ ) {
        keys_found.push_back(key);
    }

    level_info.push_back(node->children_.size());

    // Print each key with tree branch lines
    for ( size_t k = 0; k < node->data_.size(); ++k ) {
        const std::string& key         = keys_found[k];
        bool               is_last_key = (k == keys_found.size() - 1);

        // Print indent for keys: one more level than node
        printTreeIndent(sstr, level_info, level_info.size());

        // Print branch: "├──" or "└──"
        (*sstr) << (is_last_key ? "└── " : "├── ");

        // Print key name with colon
        std::string pretty_key = key;
        auto        it_pretty  = node->keys_.find(key);
        if ( it_pretty != node->keys_.end() && !it_pretty->second.first.empty() ) {
            pretty_key = it_pretty->second.first;
        }
        (*sstr) << pretty_key << ": ";

        // Print value with special formatting
        outputValueToText(node->data_.at(key), sstr);

        // Print units if available
        if ( it_pretty != node->keys_.end() && !it_pretty->second.second.empty() ) {
            (*sstr) << " " << it_pretty->second.second;
        }

        (*sstr) << "\n";
    }

    auto child_count = node->children_.size();
    for ( size_t i = 0; i < child_count; i++ ) {
        // Recurse into children and update level_info
        bool last         = (i == child_count - 1);
        level_info.back() = level_info.back() - 1;
        outputNodeToTextTree(node->children_[i], sstr, last, level_info);
    }
}

void
PerfReporter::outputRecordToTextList(const PerfData* node, std::stringstream* sstr, bool header)
{
    if ( !node ) return;

    std::vector<std::pair<std::string, std::string>> key_list; // Keep it ordered correctly and use the same units

    if ( header ) {
        // Print section header
        if ( node->keys_.find(node->name_) != node->keys_.end() ) {
            (*sstr) << node->keys_.find(node->name_)->second.first << std::endl;
        }
        else {
            (*sstr) << node->name_ << std::endl;
        }

        (*sstr) << "Record";

        for ( auto& key : node->keys_ ) {
            if ( key.first != node->name_ ) {
                if ( key.second.first.empty() )
                    (*sstr) << ", " << key.first;
                else
                    (*sstr) << ", " << key.second.first;
                key_list.push_back(std::make_pair(key.first, key.second.second));
            }
        }
        (*sstr) << std::endl;
    }
    else if ( node->parent_ == nullptr ) {
        // Just generate key list
        for ( auto& key : node->keys_ ) {
            if ( key.first != node->name_ ) {
                key_list.push_back(std::make_pair(key.first, key.second.second));
            }
        }
    }

    // Assumes a flat structure - this node has a set of children, each of which provides a list item
    for ( auto& record : node->children_ ) {
        (*sstr) << record->name_;
        for ( auto& key : key_list ) {
            (*sstr) << ", ";
            auto it = record->data_.find(key.first);
            if ( it != record->data_.end() ) {
                outputValueToText(it->second, sstr);
                if ( key.second != "" ) (*sstr) << " " << key.second;
            }
        }
        (*sstr) << std::endl;
    }
}


void
PerfReporter::outputRecordToJSON(const PerfData* node, json::ordered_json* json_obj)
{
    for ( auto kv : node->data_ ) {
        std::string valstr = convertValueToString(kv.second);
        auto        it     = node->keys_.find(kv.first);

        if ( it != node->keys_.end() && it->second.second != "" ) {
            valstr += " " + it->second.second;
        }
        (*json_obj)[kv.first] = valstr;
    }

    for ( auto child : node->children_ ) {
        auto record = nlohmann::ordered_json::object();
        outputRecordToJSON(child, &record);
        (*json_obj)[child->name_] = record;
    }
}

size_t
PerfReporter::recordCount()
{
    return records_.size();
}

} // namespace SST::Util

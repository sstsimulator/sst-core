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

#include "smartTextFormatter.h"

#include <cstddef>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

namespace SST::Util {

void
SmartTextFormatter::clear()
{
    output_.clear();
    spaces_.clear();
    word_.clear();
    current_position_ = 0;

    indent_.clear();
    indent_.push_back(0);
}


SmartTextFormatter::SmartTextFormatter(const std::vector<int>& tabStops, int repeat) :
    terminal_width_(getTerminalWidth())
{
    setTabStops(tabStops, repeat);
}


void
SmartTextFormatter::setTabStops(const std::vector<int>& stops, int repeat)
{
    if ( repeat < 1 ) {
        tab_stops_ = stops;
        return;
    }

    // Generate the tab stop "pattern" (distances between adjacent
    // stops)
    std::vector<int> distances;
    distances.push_back(stops[0]);
    for ( size_t i = 1; i < stops.size(); ++i ) {
        distances.push_back(stops[i] - stops[i - 1]);
    }

    // Clear current tabstops
    tab_stops_.clear();

    // Track the index where the repeat will start each time through
    size_t repeat_index = stops.size() - repeat;

    // Track the current index in the array
    size_t index    = 0;
    int    position = distances[0];
    do {
        tab_stops_.push_back(position);
        index++;
        if ( index == distances.size() ) {
            index = repeat_index;
        }
        position = position + distances[index];
    } while ( position < terminal_width_ );
}


void
SmartTextFormatter::append(const std::string& input)
{

    for ( size_t i = 0; i < input.length(); ++i ) {
        unsigned char current_char = input[i];

        if ( std::isspace(current_char) ) {

            // First, check to see if we were most recently
            // collecting a word. If so, we need to add it to the
            // formatted string, then we can proceed to processing
            // the current character.
            if ( word_.size() > 0 ) {
                if ( current_position_ < terminal_width_ ) {
                    // We can put the word on this line. Put in
                    // all the spaces we collected plus the word
                    output_ += spaces_;
                    output_ += word_;
                    spaces_ = "";
                    word_   = "";
                }
                else {
                    // Need to wrap the word.  Throw away any
                    // preceeding spaces, put in a newline, do the
                    // indent, then add the word
                    output_ += '\n';
                    spaces_ = "";

                    output_ += std::string(indent_.back(), ' ');
                    current_position_ = indent_.back();

                    output_ += word_;
                    current_position_ += word_.size();
                    word_ = "";
                }
            }

            // Handle the whitespace characters tab (\t), vertical tab
            // (\v), carriage return (\r), and newline (\n) as special
            // case.  All other whitespace will become space.

            // Handle tab characters
            if ( current_char == '\t' ) {
                last_char_vert_tab_ = false;
                int num_spaces      = nextTabStop(current_position_);
                if ( num_spaces == -1 ) {
                    // Hit end of line, just wrap to next line,
                    // obeying indent
                    output_ += '\n';
                    spaces_ = "";
                    if ( indent_.back() != 0 ) output_ += std::string(indent_.back(), ' ');
                    current_position_ = indent_.back();
                    continue;
                }
                // Add to the spaces object
                spaces_ += std::string(num_spaces, ' ');
                current_position_ += num_spaces;
            }

            // Handle vertical tab characters.  This will simply set
            // the indent to the currentPosition
            else if ( current_char == '\v' ) {
                if ( last_char_vert_tab_ ) {
                    // If the last character was a vertical tab,
                    // then we were actually supposed to pop the
                    // current indent.  However, we actually added
                    // another indent so need to pop two.
                    indent_.pop_back();
                    // If someone put in two \v's before a \v,
                    // then we will have an empty vector if we pop
                    // again.  Need to check for that.
                    if ( indent_.size() != 1 ) indent_.pop_back();
                    last_char_vert_tab_ = false;
                }
                else {
                    indent_.push_back(current_position_);
                    last_char_vert_tab_ = true;
                }
                for ( size_t i = 0; i < indent_.size(); ++i ) {}
                continue;
            }

            // Handle carriage return characters, which will cause
            // a new line that returns to the indent point
            else if ( current_char == '\r' ) {
                last_char_vert_tab_ = false;
                output_ += '\n';
                output_ += std::string(indent_.back(), ' ');
                spaces_           = "";
                current_position_ = indent_.back();
            }

            // Handle new line characters
            else if ( current_char == '\n' ) {
                output_ += '\n';
                spaces_           = "";
                current_position_ = 0;
                // Explicit new line wipes the indent
                indent_.clear();
                indent_.push_back(0);
            }

            // All others become a single space
            else {
                last_char_vert_tab_ = false;
                spaces_ += ' ';
                current_position_++;
            }
        }
        else {
            // For non space characters, we just add to the current word
            last_char_vert_tab_ = false;
            word_ += current_char;
            current_position_++;
        }
    }
}


std::string
SmartTextFormatter::str()
{
    std::string ret = output_;
    // Need to handle any left over word
    if ( word_.size() > 0 ) {
        if ( current_position_ < terminal_width_ ) {
            // We can put the word on this line. Put in
            // all the spaces we collected plus the word
            ret += spaces_;
            ret += word_;
        }
        else {
            // Need to wrap the word.  Throw away an
            // preceeding spaces, put in a newline, do the
            // indent, then add the word
            ret += '\n';
            ret += std::string(indent_.back(), ' ');
            ret += word_;
        }
    }
    return ret;
}

int
SmartTextFormatter::nextTabStop(int position)
{
    for ( int tab : tab_stops_ ) {
        if ( tab > position ) {
            return tab - position;
        }
    }
    // If no more tab stops, return -1
    return -1;
}

int
SmartTextFormatter::getTerminalWidth()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    // Default to 80 if unable to get width
    return w.ws_col > 0 ? w.ws_col : 80;
}

} // namespace SST::Util

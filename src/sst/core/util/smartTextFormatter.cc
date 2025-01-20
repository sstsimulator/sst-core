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

#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>


using namespace std;

namespace SST {
namespace Util {

void
SmartTextFormatter::clear()
{
    output.clear();
    spaces.clear();
    word.clear();
    currentPosition = 0;

    indent.clear();
    indent.push_back(0);
}


SmartTextFormatter::SmartTextFormatter(const std::vector<int>& tabStops, int repeat) : terminalWidth(getTerminalWidth())
{
    setTabStops(tabStops, repeat);
}


void
SmartTextFormatter::setTabStops(const std::vector<int>& stops, int repeat)
{
    if ( repeat < 1 ) {
        tabStops = stops;
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
    tabStops.clear();

    // Track the index where the repeat will start each time through
    size_t repeat_index = stops.size() - repeat;

    // Track the current index in the array
    size_t index    = 0;
    int    position = distances[0];
    do {
        tabStops.push_back(position);
        index++;
        if ( index == distances.size() ) { index = repeat_index; }
        position = position + distances[index];
    } while ( position < terminalWidth );
}


void
SmartTextFormatter::append(const std::string& input)
{

    for ( size_t i = 0; i < input.length(); ++i ) {
        unsigned char currentChar = input[i];

        if ( std::isspace(currentChar) ) {

            // First, check to see if we were most recently
            // collecting a word. If so, we need to add it to the
            // formatted string, then we can proceed to processing
            // the current character.
            if ( word.size() > 0 ) {
                if ( currentPosition < terminalWidth ) {
                    // We can put the word on this line. Put in
                    // all the spaces we collected plus the word
                    output += spaces;
                    output += word;
                    spaces = "";
                    word   = "";
                }
                else {
                    // Need to wrap the word.  Throw away any
                    // preceeding spaces, put in a newline, do the
                    // indent, then add the word
                    output += '\n';
                    spaces = "";

                    output += std::string(indent.back(), ' ');
                    currentPosition = indent.back();

                    output += word;
                    currentPosition += word.size();
                    word = "";
                }
            }

            // Handle the whitespace characters tab (\t), vertical tab
            // (\v), carriage return (\r), and newline (\n) as special
            // case.  All other whitespace will become space.

            // Handle tab characters
            if ( currentChar == '\t' ) {
                last_char_vert_tab = false;
                int num_spaces     = nextTabStop(currentPosition);
                if ( num_spaces == -1 ) {
                    // Hit end of line, just wrap to next line,
                    // obeying indent
                    output += '\n';
                    spaces = "";
                    if ( indent.back() != 0 ) output += std::string(indent.back(), ' ');
                    currentPosition = indent.back();
                    continue;
                }
                // Add to the spaces object
                spaces += std::string(num_spaces, ' ');
                currentPosition += num_spaces;
            }

            // Handle vertical tab characters.  This will simply set
            // the indent to the currentPosition
            else if ( currentChar == '\v' ) {
                if ( last_char_vert_tab ) {
                    // If the last character was a vertical tab,
                    // then we were actually supposed to pop the
                    // current indent.  However, we actually added
                    // another indent so need to pop two.
                    indent.pop_back();
                    // If someone put in two \v's before a \v,
                    // then we will have an empty vector if we pop
                    // again.  Need to check for that.
                    if ( indent.size() != 1 ) indent.pop_back();
                    last_char_vert_tab = false;
                }
                else {
                    indent.push_back(currentPosition);
                    last_char_vert_tab = true;
                }
                for ( size_t i = 0; i < indent.size(); ++i ) {}
                continue;
            }

            // Handle carriage return characters, which will cause
            // a new line that returns to the indent point
            else if ( currentChar == '\r' ) {
                last_char_vert_tab = false;
                output += '\n';
                output += std::string(indent.back(), ' ');
                spaces          = "";
                currentPosition = indent.back();
            }

            // Handle new line characters
            else if ( currentChar == '\n' ) {
                output += '\n';
                spaces          = "";
                currentPosition = 0;
                // Explicit new line wipes the indent
                indent.clear();
                indent.push_back(0);
            }

            // All others become a single space
            else {
                last_char_vert_tab = false;
                spaces += ' ';
                currentPosition++;
            }
        }
        else {
            // For non space characters, we just add to the current word
            last_char_vert_tab = false;
            word += currentChar;
            currentPosition++;
        }
    }
}


std::string
SmartTextFormatter::str()
{
    std::string ret = output;
    // Need to handle any left over word
    if ( word.size() > 0 ) {
        if ( currentPosition < terminalWidth ) {
            // We can put the word on this line. Put in
            // all the spaces we collected plus the word
            ret += spaces;
            ret += word;
        }
        else {
            // Need to wrap the word.  Throw away an
            // preceeding spaces, put in a newline, do the
            // indent, then add the word
            ret += '\n';
            ret += std::string(indent.back(), ' ');
            ret += word;
        }
    }
    return ret;
}

int
SmartTextFormatter::nextTabStop(int position)
{
    for ( int tab : tabStops ) {
        if ( tab > position ) { return tab - position; }
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

} // namespace Util
} // namespace SST

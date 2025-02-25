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

#ifndef SST_CORE_UTIL_SMARTTEXTFORMATTER_H
#define SST_CORE_UTIL_SMARTTEXTFORMATTER_H

#include <string>
#include <vector>

namespace SST::Util {

/**
   Class to format text for console output.  It will wrap lines at
   word boundaries based on the terminal width.  It can also handle
   tab and indent using various escape sequences described in the
   append() function.

   NOTE: The API is in flux and won't be final until the SST 15
   release.
 */
class SmartTextFormatter
{

    /**
       List of tab stops
     */
    std::vector<int> tab_stops_;

    /**
       Width of the terminal.  Lines will be wrapped at this width
     */
    int terminal_width_;

    /**
       The output string that is being built up with calls to append
     */
    std::string output_;

    /**
       Used to hold the current whitespace between words
     */
    std::string spaces_;

    /**
       Used to hold the current word
     */
    std::string word_;

    /**
       Position (column) that the next character will write at in the
       current line
     */
    int current_position_ = 0;

    /**
       True if the last character was a vertical tab.  This is needed
       because two vertical tabs in a row will pop the most recent
       indent
     */
    bool last_char_vert_tab_ = false;

    /**
       Stack of indents.  Add an indent at the currentPosition by
       using \v.  Pop the most recent indent with \v\v.
     */
    std::vector<size_t> indent_ = { 0 };

public:
    /**
       Constructor for SmartTextFormatter

       @param tabStops Vector of column indices that will serve as tab
       stops

       @param repeat If set to value greater than zero, the last
       "repeat" tab stops will repeat the input pattern through the
       whole width of the terminal (i.e., the differences from the tab
       stop before will be used to generate new tab stops until the
       end of the line).
     */
    SmartTextFormatter(const std::vector<int>& tabStops, int repeat = 0);

    /**
       Clear the formatter
    */
    void clear();

    /**
       Sets the tabstops for the formatter. This can be done at
       anytime and will take immediate effect.

       @param stops Vector of column indexes that will serve as tab
       stops

       @param repeat If set to value greater than zero, the last
       "repeat" tab stops will repeat the input pattern through the
       whole width of the terminal (i.e., the differences from the tab
       stop before will be used to generate new tab stops until the
       end of the line).
     */
    void setTabStops(const std::vector<int>& stops, int repeat = 0);

    /**
       Append a string to the formatter.  The formatter will add the
       string to the output (which can be retrieved through the str()
       function) and add newline characters between words to wrap text
       at the terminal boundary.  The width of the terminal is
       determined automatically and will default to 80 if the width is
       indeterminate (for example, the output is piped to a file).

       The input string can also include the following escape
       sequences, which will behave as described:

       \t - Will advance the output to the next tab stop.  If the
       current line is already beyond the current tab stop, the
       formatter will just insert a newline

       \v - Will push a new indent at the current column of the output
       line to the indent stack

       \v\v - Will pop the most recent indent from the stack

       \n - Normal new line character.  In addition to adding a new
       line, it will also clear the indent stack

       \r - Will insert a new line character, but advance the output
       to the current indent position

       @param input Input string to add to the formatted output
     */
    void append(const std::string& input);


    /**
       Return the current output of the formatter. Any trailing spaces
       will be left off.
     */
    std::string str();

private:
    /**
       Will return the number of spaces needed to get to next tab stop
     */
    int nextTabStop(int position);

    int getTerminalWidth();
};


} // namespace SST::Util

#endif // SST_CORE_UTIL_SMARTTEXTFORMATTER_H

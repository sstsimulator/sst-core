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

#include "sst/core/impl/interactive/cmdLineEditor.h"
#include <cctype>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include "cmdLineEditor.h"

void CmdLineEditor::setRawMode() {
    termios rawTerm;
    // Get current terminal settings
    tcgetattr(STDIN_FILENO, &originalTerm); 
    rawTerm = originalTerm;
    // Disable canonical mode and echoing
    rawTerm.c_lflag &= ~(ICANON | ECHO);
    // Apply new settings
    tcsetattr(STDIN_FILENO, TCSANOW, &rawTerm); 
}
void CmdLineEditor::restoreTermMode() {
    // Restore original settings
    tcsetattr(STDIN_FILENO, TCSANOW, &originalTerm); 
}
bool CmdLineEditor::read2chars(char seq[3]) {
    bool ok = read(STDIN_FILENO, &seq[0], 1);
    if (!ok) return 0;
    ok = read(STDIN_FILENO, &seq[1], 1);
    seq[2] = '\0';
    return ok;
}
void CmdLineEditor::move_cursor_left() {
    if (curpos>(int)prompt.size()+1) {
        std::cout << move_left_ctl << std::flush;
        curpos--;
    }
}
void CmdLineEditor::move_cursor_right(int slen) {
    if (curpos < (int)prompt.size() + slen + 1) {
        std::cout << move_right_ctl << std::flush;
        curpos++;
    }
}

void
CmdLineEditor::set_cmd_strings(const std::list<std::string>& sortedStrings)
{  
    cmdStrings = sortedStrings;
}

bool CmdLineEditor::selectMatches(const std::list<std::string>& list, const std::string& searchfor, std::vector<std::string>& matches, std::string& newtoken) {
    std::copy_if(
        list.begin(), list.end(),
        std::back_inserter(matches),
        [&searchfor](const std::string& s) {
            return matchBeginStringCaseInsensitive(searchfor, s);
        });
    if (matches.size()==1) {
        // unique. Set token to this complete string
        newtoken = matches[0] + " ";
        return true;
    } else if (matches.size()>0) {
        // list all matching strings
        std::cout << "\n";
        for (const std::string& s : matches)
            std::cout << s << " ";
        std::cout << std::endl;
    }
    return false;
}

void
CmdLineEditor::auto_complete(std::string& cmd) {
    bool hasTrailingSpace = (!cmd.empty() && cmd.back() == ' ');
    std::istringstream iss(cmd);
    std::string token;
    std::vector<std::string> tokens;
    while (iss >> token)
        tokens.push_back(token);
    if (tokens.size() == 0 ) {
        // list all command strings
        if (cmdStrings.size()>0) {
            std::cout << "\n";
            for ( const std::string& s : cmdStrings )
                std::cout << s << " ";
            // TODO std::cout << move_up_ctl << std::flush;
            // Cleaner for now
            std::cout << std::endl;
        }
    } else if (tokens.size() == 1 && !hasTrailingSpace) {
        // find all matching command strings starting with tokens[0]
        std::vector<std::string> matches;
        bool exact_match = selectMatches(cmdStrings, tokens[0], matches, cmd);
        if (exact_match) {
            curpos = cmd.size() + prompt.size() + 1;
            return;
        }
    } else {
        // after 1st token. provide matching strings in object map
        if (this->listing_callback_ == nullptr) return;
        std::list<std::string> listing;
        listing_callback_(listing);
        if (listing.size()==0) return;
        if (hasTrailingSpace) {
            //list everything
            std::cout << "\n";
            for ( const std::string& s : listing ) 
                std::cout << s << " ";
            std::cout << std::endl;
        } else {
            std::vector<std::string> matches;
            std::string newtoken;
            bool exact_match = selectMatches(listing, tokens[tokens.size()-1], matches, newtoken);
            if (exact_match) {
                std::stringstream s;
                for (size_t i=0; i<tokens.size()-1; i++)
                    s << tokens[i] << " ";
                s << newtoken;
                cmd = s.str();
                curpos = cmd.size() + prompt.size() + 1;
                return;
            }
        }
    }
}



void
CmdLineEditor::redraw_line(const std::string& s)
{
    std::cout << prompt_clear << s << esc_ctl << curpos << "G" << std::flush;
};

void
CmdLineEditor::getline(const std::vector<std::string>& cmdHistory, std::string& newcmd)
{

    // save terminal settings and enable raw mode
    this->setRawMode();
    
    // local edited history
    std::vector<std::string> history = cmdHistory;
    history.emplace_back("");           // current command entry
    int max = history.size() - 1;       // maximum index in history vector
    int index = max;                    // position in history vector

    // initial empty prompt
    std::cout << prompt_clear << history[index] << std::flush;
    curpos = history[index].size() + prompt.size() + 1;
    
    // Start checking for keys
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == lf_char) {
            // Done if line feed
            break; 
        } else if (c == esc_char) { 
            // Escape character
            char seq[3];
            if (this->read2chars(seq)) {
                std::string key(seq);
                if ( key == arrow_up ) {
                    if (index > 0) index--;
                    std::cout << prompt_clear << history[index] << std::flush;
                    curpos = history[index].size() + prompt.size()+ 1;
                } else if ( key == arrow_dn ) {
                    if (index < max) index++;
                    std::cout << prompt_clear << history[index] << std::flush;
                    curpos = history[index].size() + prompt.size() + 1;
                } else if ( key == arrow_lf) {
                    move_cursor_left();
                } else if ( key == arrow_rt) {
                    move_cursor_right(history[index].size());
                }
            }
        } else if (c>=32 && c<127) {
            if (curpos >= max_line_size) continue;
            // Insert printable character
            int position = history[index].size()==0 ? 0 : curpos - 1 - prompt.size();
            if (position < 0 || position > (int) history[index].size() ) {
                std::cout << prompt_clear << position << std::flush;
                continue; // something went wrong
            }
            history[index].insert(position, 1, c);
            curpos++;
            redraw_line(history[index]);
        } else if ( c == bs_char ) {
            // backspace. Delete a character to the left
            if (curpos <= (int)prompt.size() + 1 ) continue;
            curpos--;
            int position = curpos - prompt.size()-1;
            if (position < 0 || position >= (int) history[index].size() ) {
                continue;
            }
            history[index].erase(position, 1);
            redraw_line(history[index]);
        } else if ( c == ctrl_d) {
            // delete character at cursor
            int position = curpos - prompt.size()-1;
            if (position < 0 || position >= (int) history[index].size() ) {
                continue;
            }
            history[index].erase(position,1);
            redraw_line(history[index]);
        } else if ( c == ctrl_a) {
            // move cursor to start of line
            curpos = prompt.size()+ 1;
            redraw_line(history[index]);
        } else if ( c == ctrl_e ) {
            // move cursor to the end of the line
            curpos = history[index].size() + prompt.size()+ 1;
            std::cout << prompt_clear << history[index] << std::flush;
        } else if ( c == ctrl_k) {
            // remove characters from the cursor to the end of the line
            int position = history[index].size()==0 ? 0 : curpos - 1 - prompt.size();
            if (position < 0 || position >= (int) history[index].size() ) 
                continue; // something went wrong
            int num2erase = history[index].size() - curpos;
            // std::cout << num2erase << std::endl;
            if (num2erase < 0 || num2erase > (int) history[index].size())
                continue; // something else went wrong
            history[index].erase(position);
            curpos = history[index].size() + prompt.size()+ 1;
            std::cout << prompt_clear << history[index] << std::flush;
        } else if ( c == ctrl_b ) {
            move_cursor_left();
        } else if ( c == ctrl_f ) {
            move_cursor_right(history[index].size());
        } else if ( c == tab_char ) {
            auto_complete(history[index]);
            redraw_line(history[index]);
        }
    }

    // Restore original terminal settings
    this->restoreTermMode();

    // set the new line info
    newcmd = history[index];
    std::cout << std::endl;
}

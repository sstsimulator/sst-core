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
#include <iostream>
#include <unistd.h>

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

void CmdLineEditor::getline(const std::vector<std::string> &cmdHistory, std::string &newcmd) {
    
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
            std::cout << prompt_clear << history[index] << esc_ctl << curpos << "G" << std::flush;
        } else if ( c == bs_char ) {
            // backspace. Delete a character to the left
            if (curpos <= (int)prompt.size() + 1 ) continue;
            curpos--;
            int position = curpos - prompt.size()-1;
            if (position < 0 || position >= (int) history[index].size() ) {
                continue;
            }
            history[index].erase(position, 1);
            std::cout << prompt_clear << history[index] << esc_ctl << curpos << "G" << std::flush;
        } else if ( c == ctrl_d) {
            // delete character at cursor
            int position = curpos - prompt.size()-1;
            if (position < 0 || position >= (int) history[index].size() ) {
                continue;
            }
            history[index].erase(position,1);
            std::cout << prompt_clear << history[index] << esc_ctl << curpos << "G" << std::flush;
        } else if ( c == ctrl_a) {
            // move cursor to start of line
            curpos = prompt.size()+ 1;
            std::cout << prompt_clear << history[index] << esc_ctl << curpos << "G" << std::flush;
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
        }
    }

    // Restore original terminal settings
    this->restoreTermMode();

    // set the new line info
    newcmd = history[index];
    std::cout << std::endl;
}

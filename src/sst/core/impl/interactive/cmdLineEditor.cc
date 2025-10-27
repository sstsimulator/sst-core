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

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iterator>
#include <sstream>

// #define _KEYB_DEBUG_

// only use termios read/write functions for console!

CmdLineEditor::CmdLineEditor()
{
#ifdef _KEYB_DEBUG_
    dbgFile.open("_keyb_debug_.out");
#endif
}

int
CmdLineEditor::checktty(int rc)
{
    if ( rc == -1 ) {
        std::string msg("input error: ");
        msg = msg + strerror(errno) + "\n";
        writeStr(msg);
        errno = 0;
    }
    return rc;
}

int
CmdLineEditor::setRawMode()
{
    termios rawTerm;
    errno  = 0;
    int rc = checktty(tcgetattr(STDIN_FILENO, &originalTerm));
    if ( rc ) return rc;
    rawTerm = originalTerm;
    // Disable canonical mode and echoing
    rawTerm.c_lflag &= ~(ICANON | ECHO);
    // Apply new settings
    rc = checktty(tcsetattr(STDIN_FILENO, TCSANOW, &rawTerm));
    return rc;
}

int
CmdLineEditor::restoreTermMode()
{
    // Restore original settings
    int rc = checktty(tcsetattr(STDIN_FILENO, TCSANOW, &originalTerm));
    return rc;
}

bool
CmdLineEditor::read2chars(char (&seq)[3])
{
    auto nbytes = read(STDIN_FILENO, &seq[0], 1);
    if ( !nbytes ) {
#ifdef _KEYB_DEBUG_
        dbgFile << "oops: read2chars tried to read nbytes\n";
#endif
        return false;
    }
    nbytes = read(STDIN_FILENO, &seq[1], 1);
    if ( !nbytes ) {
#ifdef _KEYB_DEBUG_
        dbgFile << "oops: read2chars missing 2nd nbyte\n";
#endif
        return false;
    }
    seq[2] = '\0';
    return (nbytes > 0);
}

void
CmdLineEditor::move_cursor_left()
{
    if ( curpos > (int)prompt.size() + 1 ) {
        writeStr(move_left_ctl);
        curpos--;
    }
}

void
CmdLineEditor::move_cursor_right(int slen)
{
    if ( curpos < (int)prompt.size() + slen + 1 ) {
        writeStr(move_right_ctl);
        curpos++;
    }
}

void
CmdLineEditor::set_cmd_strings(const std::list<std::string>& sortedStrings)
{
    cmdStrings = sortedStrings;
}

bool
CmdLineEditor::selectMatches(const std::list<std::string>& list, const std::string& searchfor,
    std::vector<std::string>& matches, std::string& newtoken)
{
    std::copy_if(list.begin(), list.end(), std::back_inserter(matches),
        [&searchfor](const std::string& s) { return matchBeginStringCaseInsensitive(searchfor, s); });
    if ( matches.size() == 1 ) {
        // unique. Set token to this complete string
        newtoken = matches[0] + " ";
        return true;
    }
    else if ( matches.size() > 0 ) {
        // list all matching strings
        writeStr("\n");
        for ( const std::string& s : matches ) {
            writeStr(s);
            writeStr(" ");
        }
        writeStr("\n");
    }
    return false;
}

void
CmdLineEditor::flush_bad_escape()
{
    char c;
    int  max = 4;
    while ( read(STDIN_FILENO, &c, 1) ) {
        if ( max-- <= 0 ) break;
#ifdef _KEYB_DEBUG_
        dbgFile << "Discarding: " << std::hex << (int)c << std::endl;
#endif
    }
}

void
CmdLineEditor::auto_complete(std::string& cmd)
{
    bool                     hasTrailingSpace = (!cmd.empty() && cmd.back() == ' ');
    std::istringstream       iss(cmd);
    std::string              token;
    std::vector<std::string> tokens;
    while ( iss >> token )
        tokens.push_back(token);
    if ( tokens.size() == 0 ) {
        // list all command strings
        if ( cmdStrings.size() > 0 ) {
            writeStr("\n");
            for ( const std::string& s : cmdStrings ) {
                writeStr(s);
                writeStr(" ");
            }
            writeStr("\n");
        }
    }
    else if ( tokens.size() == 1 && !hasTrailingSpace ) {
        // find all matching command strings starting with tokens[0]
        std::vector<std::string> matches;
        bool                     exact_match = selectMatches(cmdStrings, tokens[0], matches, cmd);
        if ( exact_match ) {
            curpos = cmd.size() + prompt.size() + 1;
#ifdef _KEYB_DEBUG_
// dbgFile << "prompt&" << &prompt << "'" << prompt << "'(" << prompt.size() << ") "
//          << "cmd&" << &cmd << "'" << cmd << "'(" << cmd.size() << ")" << std::endl;
#endif
            return;
        }
    }
    else {
        // after 1st token. provide matching strings in object map
        if ( this->listing_callback_ == nullptr ) return;
        std::list<std::string> listing;
        listing_callback_(listing);
        if ( listing.size() == 0 ) return;
        if ( hasTrailingSpace ) {
            // list everything
            writeStr("\n");
            for ( const std::string& s : listing ) {
                writeStr(s);
                writeStr(" ");
            }
            writeStr("\n");
        }
        else {
            std::vector<std::string> matches;
            std::string              newtoken;
            bool                     exact_match = selectMatches(listing, tokens[tokens.size() - 1], matches, newtoken);
            if ( exact_match ) {
                std::stringstream s;
                for ( size_t i = 0; i < tokens.size() - 1; i++ )
                    s << tokens[i] << " ";
                s << newtoken;
                cmd    = s.str();
                curpos = cmd.size() + prompt.size() + 1;
                return;
            }
        }
    }
}

void
CmdLineEditor::redraw_line(const std::string& s)
{
    std::stringstream line;
    line << prompt_clear << s << esc_ctl << curpos << 'G';
    writeStr(line.str());
#ifdef _KEYB_DEBUG_
// dbgFile << curpos << " " << s.length() << std::endl;
#endif
};

void
CmdLineEditor::getline(const std::vector<std::string>& cmdHistory, std::string& newcmd)
{

    // save terminal settings and enable raw mode
    if ( setRawMode() == -1 ) return;

    // local edited history
    std::vector<std::string> history = cmdHistory;
    history.emplace_back("");       // current command entry
    int max   = history.size() - 1; // maximum index in history vector
    int index = max;                // position in history vector

    // initial empty prompt
    std::stringstream prompt_line;
    prompt_line << prompt_clear << history[index];
    writeStr(prompt_line.str());
    curpos = history[index].size() + prompt.size() + 1;

    // Start checking for keys
    char              c;
    int               bytesRead = 1;
    std::stringstream oline;
    while ( (bytesRead = read(STDIN_FILENO, &c, 1)) == 1 ) {
#ifdef _KEYB_DEBUG_
        dbgFile << std::hex << (int)c << std::endl;
#endif
        if ( c == lf_char ) {
            // Done if line feed
            break;
        }
        else if ( c == esc_char ) {
            // Escape character
            char seq[3];
            if ( this->read2chars(seq) ) {
                std::string key(seq);
                if ( key == arrow_up ) {
                    if ( index > 0 ) index--;
                    oline << prompt_clear << history[index];
                    writeStr(oline.str());
                    curpos = history[index].size() + prompt.size() + 1;
                }
                else if ( key == arrow_dn ) {
                    if ( index < max ) index++;
                    oline << prompt_clear << history[index];
                    writeStr(oline.str());
                    curpos = history[index].size() + prompt.size() + 1;
                }
                else if ( key == arrow_lf ) {
                    move_cursor_left();
                }
                else if ( key == arrow_rt ) {
                    move_cursor_right(history[index].size());
                }
                else {
// unknown escape sequence ( TODO: longer than 2 escape sequences )
#ifdef _KEYB_DEBUG_
                    dbgFile << "Unhandled escape sequence\n" << std::endl;
#endif
                    flush_bad_escape();
                }
            }
            else {
#ifdef _KEYB_DEBUG_
                dbgFile << "read2chars failed\n" << std::endl;
#endif
            }
        }
        else if ( c >= 32 && c < 127 ) {
            if ( curpos >= max_line_size ) continue;
            // Insert printable character
            int position = history[index].size() == 0 ? 0 : curpos - 1 - prompt.size();
            if ( position < 0 || position > (int)history[index].size() ) {
                oline << prompt_clear << position;
                writeStr(oline.str());
                continue; // something went wrong
            }
            history[index].insert(position, 1, c);
            curpos++;
            redraw_line(history[index]);
        }
        else if ( c == bs_char ) {
            // backspace. Delete a character to the left
            if ( curpos <= (int)prompt.size() + 1 ) continue;
            curpos--;
            int position = curpos - prompt.size() - 1;
            if ( position < 0 || position >= (int)history[index].size() ) {
                continue;
            }
            history[index].erase(position, 1);
            redraw_line(history[index]);
        }
        else if ( c == ctrl_d ) {
            // delete character at cursor
            int position = curpos - prompt.size() - 1;
            if ( position < 0 || position >= (int)history[index].size() ) {
                continue;
            }
            history[index].erase(position, 1);
            redraw_line(history[index]);
        }
        else if ( c == ctrl_a ) {
            // move cursor to start of line
            curpos = prompt.size() + 1;
            redraw_line(history[index]);
        }
        else if ( c == ctrl_e ) {
            // move cursor to the end of the line
            curpos = history[index].size() + prompt.size() + 1;
            oline << prompt_clear << history[index];
            writeStr(oline.str());
        }
        else if ( c == ctrl_k ) {
            // remove characters from the cursor to the end of the line
            int position = history[index].size() == 0 ? 0 : curpos - 1 - prompt.size();
            if ( position < 0 || position >= (int)history[index].size() ) continue; // something went wrong
            int num2erase = history[index].size() - curpos;
            // std::cout << num2erase << std::endl;
            if ( num2erase < 0 || num2erase > (int)history[index].size() ) continue; // something else went wrong
            history[index].erase(position);
            curpos = history[index].size() + prompt.size() + 1;
            oline << prompt_clear << history[index];
            writeStr(oline.str());
        }
        else if ( c == ctrl_b ) {
            move_cursor_left();
        }
        else if ( c == ctrl_f ) {
            move_cursor_right(history[index].size());
        }
        else if ( c == tab_char ) {
            auto_complete(history[index]);
            redraw_line(history[index]);
        }
#ifdef _KEYB_DEBUG_
        else {
            dbgFile << "Unhandled char " << std::hex << c << std::endl;
        }
#endif
    }

    if ( bytesRead == -1 ) {
        oline << "input error: " << strerror(errno);
        writeStr(oline.str());
        errno = 0;
    }

    // Restore original terminal settings
    this->restoreTermMode();

    // set the new line info
    newcmd = history[index];
    writeStr("\n");
}

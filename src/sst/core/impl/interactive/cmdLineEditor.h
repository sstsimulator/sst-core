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

#ifndef SST_CORE_IMPL_INTERACTIVE_CMDLINEEDITOR_H
#define SST_CORE_IMPL_INTERACTIVE_CMDLINEEDITOR_H

#include <cctype>
#include <fstream>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>

/**
 * Common write to console
 */
inline void
writeStr(const std::string& msg)
{
    (void)!write(STDOUT_FILENO, msg.data(), msg.size());
}

/**
 * The command line editor uses termios to detect key presses
 * and perform auto-completions. Upon entering the editor,
 * the current terminal settings are saved and we enter a
 * "raw" terminal mode. While in raw terminal mode it is
 * critical to ensure that exclusivel use read and write commands
 * for std:out access. Mixing iostream access can corrupt the
 * buffers.
 */
class CmdLineEditor
{
public:
    static constexpr char esc_char = '\x1B';
    static constexpr char tab_char = '\x9';
    static constexpr char lf_char  = '\xa';
    static constexpr char bs_char  = '\x7f';
    static constexpr char ctrl_a   = '\x1';
    static constexpr char ctrl_b   = '\x2';
    static constexpr char ctrl_d   = '\x4';
    static constexpr char ctrl_e   = '\x5';
    static constexpr char ctrl_f   = '\x6';
    static constexpr char ctrl_k   = '\xb';

    const std::string                        arrow_up    = "[A";
    const std::string                        arrow_dn    = "[B";
    const std::string                        arrow_rt    = "[C";
    const std::string                        arrow_lf    = "[D";
    const std::map<std::string, std::string> arrowKeyMap = {
        { arrow_up, "Up" },
        { arrow_dn, "Down" },
        { arrow_rt, "Right" },
        { arrow_lf, "Left" },
    };

    const std::string clear_line_ctl = "\x1B[2K";
    const std::string move_left_ctl  = "\x1B[1D";
    const std::string move_right_ctl = "\x1B[1C";
    const std::string esc_ctl        = "\x1B["; //"\x1b[5G" moves to column 5
    const std::string move_up_ctl    = "\x1B[1F";

    const std::string prompt       = "> ";
    const std::string prompt_clear = "\x1B[2K\r> ";

    static constexpr int max_line_size = 2048;

    CmdLineEditor();
    virtual ~CmdLineEditor() = default;
    void redraw_line(const std::string& s);
    void getline(const std::vector<std::string>& cmdHistory, std::string& newcmd);

    // Auto-completion support
    void set_cmd_strings(const std::list<std::string>& sortedStrings);
    void set_listing_callback(std::function<void(std::list<std::string>&)> callback) { listing_callback_ = callback; }

    // debug helper
    std::ofstream dbgFile;

private:
    termios                                               originalTerm;
    std::list<std::string>                                cmdStrings        = {};
    std::function<void(std::list<std::string>& callback)> listing_callback_ = nullptr;

    int  curpos = -1;
    int  checktty(int rc);
    int  setRawMode();
    int  restoreTermMode();
    bool read2chars(char (&seq)[3]);
    void move_cursor_left();
    void move_cursor_right(int slen);
    void auto_complete(std::string& cmd);
    bool selectMatches(const std::list<std::string>& list, const std::string& searchfor,
        std::vector<std::string>& matches, std::string& newcmd);

private:
    // yet more string helpers
    static bool compareCharCaseInsensitive(char c1, char c2)
    {
        return std::tolower(static_cast<unsigned char>(c1)) == std::tolower(static_cast<unsigned char>(c2));
    };

    // exact, but case insenstive, match
    // static bool compareStringCaseInsensitive(const std::string& s1, const std::string& s2) {
    //     if (s1.length() != s2.length())
    //         return false;
    //     return std::equal(s1.begin(), s1.end(), s2.begin(), compareCharCaseInsensitive);
    // };

    // match if begining of second string matches the first
    static bool matchBeginStringCaseInsensitive(const std::string& searchfor, const std::string& longstr)
    {
        if ( longstr.length() < searchfor.length() ) return false;
        std::string matchstr = longstr.substr(0, searchfor.length());
        return std::equal(searchfor.begin(), searchfor.end(), matchstr.begin(), compareCharCaseInsensitive);
    }

    // Bug: Why is this escape code injected at the 7th char after verbose printing of 0x...?
    void flush_bad_escape();
};

#endif // SST_CORE_IMPL_INTERACTIVE_CMDLINEEDITOR_H

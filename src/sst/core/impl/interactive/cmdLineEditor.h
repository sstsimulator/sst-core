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

#include <termios.h>
#include <map> 
#include <string>
#include <vector>

class CmdLineEditor {
public:
    const char esc_char = '\x1B';
    const char tab_char = '\x9';
    const char lf_char = '\xa';
    const char bs_char = '\x7f';
    const char ctrl_a  = '\x1';
    const char ctrl_b  = '\x2';
    const char ctrl_d  = '\x4';
    const char ctrl_e  = '\x5';
    const char ctrl_f  = '\x6';
    const char ctrl_k  = '\xb';

    const std::string arrow_up = "[A";
    const std::string arrow_dn = "[B";
    const std::string arrow_rt = "[C";
    const std::string arrow_lf = "[D";
    const std::map<const std::string, const std::string> arrowKeyMap = {
        { arrow_up, "Up"    },
        { arrow_dn, "Down"  },
        { arrow_rt, "Right" },
        { arrow_lf, "Left"  },
    };

    const std::string clear_line_ctl = "\x1B[2K";
    const std::string move_left_ctl  = "\x1B[1D";
    const std::string move_right_ctl = "\x1B[1C";
    const std::string esc_ctl = "\x1B[";    //"\x1b[5G" moves to column 5

    const std::string prompt = "> ";
    const std::string prompt_clear = "\x1B[2K\r> ";

    const int max_line_size = 2048;

    void getline(const std::vector<std::string> &cmdHistory, std::string &newcmd);

private:
    termios originalTerm;
    int curpos = -1;
    void setRawMode();
    void restoreTermMode();
    bool read2chars(char seq[3]);
    void move_cursor_left();
    void move_cursor_right(int slen);
};

#endif //SST_CORE_IMPL_INTERACTIVE_CMDLINEEDITOR_H
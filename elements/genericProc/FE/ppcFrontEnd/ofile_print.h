/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

#include <ar.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/reloc.h>

/* from bytesex.h */
enum byte_sex {
    UNKNOWN_BYTE_SEX,
    BIG_ENDIAN_BYTE_SEX,
    LITTLE_ENDIAN_BYTE_SEX
};


extern void print_mach_header(
    struct mach_header *mh,
    bool verbose);

extern void print_loadcmds(
    struct mach_header *mh,
    struct load_command *load_commands,
    enum byte_sex load_commands_byte_sex,
    unsigned long object_size,
    bool verbose,
    bool very_verbose);

extern void print_segment_command(
    struct segment_command *sg,
    unsigned long object_size,
    bool verbose);

extern void print_section(
    struct section *s,
    struct segment_command *sg,
    struct mach_header *mh,
    unsigned long object_size,
    bool verbose);

extern void print_thread_states(
    char *begin, 
    char *end,
    struct mach_header *mh,
    enum byte_sex thread_states_byte_sex);

extern void print_cstring_section(
    char *sect,
    unsigned long sect_size,
    unsigned long sect_addr,
    bool print_addresses);

extern void print_literal4_section(
    char *sect,
    unsigned long sect_size,
    unsigned long sect_addr,
    enum byte_sex literal_byte_sex,
    bool print_addresses);

extern void print_literal8_section(
    char *sect,
    unsigned long sect_size,
    unsigned long sect_addr,
    enum byte_sex literal_byte_sex,
    bool print_addresses);

extern void print_literal_pointer_section(
    struct mach_header *mh,
    struct load_command *lc,
    enum byte_sex object_byte_sex,
    char *addr,
    unsigned long size,
    char *sect,
    unsigned long sect_size,
    unsigned long sect_addr,
    struct nlist *symbols,
    unsigned long nsymbols,
    char *strings,
    unsigned long strings_size,
    struct relocation_info *relocs,
    unsigned long nrelocs,
    bool print_addresses);


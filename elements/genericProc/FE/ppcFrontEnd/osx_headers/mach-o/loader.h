#ifndef _OSX_LOADER_H_
#define _OSX_LOADER_H_

#include <sst_stdint.h>

#define PPC_THREAD_STATE_COUNT ((mach_msg_type_number_t) \
   (sizeof(ppc_thread_state_t) / sizeof(int)))


#define PPC_THREAD_STATE        1
#define PPC_FLOAT_STATE         2
#define PPC_EXCEPTION_STATE     3
#define PPC_VECTOR_STATE        4
#define PPC_THREAD_STATE64      5
#define PPC_EXCEPTION_STATE64   6
#define THREAD_STATE_NONE       7

#define S_REGULAR               0x0     /* regular section */
#define S_ZEROFILL              0x1     /* zero fill on demand section */
#define S_CSTRING_LITERALS      0x2     /* section with only literal C strings*/
#define S_4BYTE_LITERALS        0x3     /* section with only 4 byte literals */
#define S_8BYTE_LITERALS        0x4     /* section with only 8 byte literals */
#define S_LITERAL_POINTERS      0x5     /* section with only pointers to */

#define VM_PROT_NONE    ((vm_prot_t) 0x00)
#define VM_PROT_READ    ((vm_prot_t) 0x01)      /* read permission */
#define VM_PROT_WRITE   ((vm_prot_t) 0x02)      /* write permission */
#define VM_PROT_EXECUTE ((vm_prot_t) 0x04)      /* execute permission */

/* Constants for the cmd field of all load commands, the type */
#define LC_SEGMENT      0x1     /* segment of this file to be mapped */
#define LC_SYMTAB       0x2     /* link-edit stab symbol table info */
#define LC_SYMSEG       0x3     /* link-edit gdb symbol table info (obsolete) */
#define LC_THREAD       0x4     /* thread */
#define LC_UNIXTHREAD   0x5     /* unix thread (includes a stack) */
#define LC_LOADFVMLIB   0x6     /* load a specified fixed VM shared library */
#define LC_IDFVMLIB     0x7     /* fixed VM shared library identification */
#define LC_IDENT        0x8     /* object identification info (obsolete) */
#define LC_FVMFILE      0x9     /* fixed VM file inclusion (internal use) */
#define LC_PREPAGE      0xa     /* prepage command (internal use) */
#define LC_DYSYMTAB     0xb     /* dynamic link-edit symbol table info */
#define LC_LOAD_DYLIB   0xc     /* load a dynamically linked shared library */
#define LC_ID_DYLIB     0xd     /* dynamically linked shared lib ident */
#define LC_LOAD_DYLINKER 0xe    /* load a dynamic linker */
#define LC_ID_DYLINKER  0xf     /* dynamic linker identification */
#define LC_PREBOUND_DYLIB 0x10  /* modules prebound for a dynamically */
                                /*  linked shared library */
#define LC_ROUTINES     0x11    /* image routines */
#define LC_SUB_FRAMEWORK 0x12   /* sub framework */
#define LC_SUB_UMBRELLA 0x13    /* sub umbrella */
#define LC_SUB_CLIENT   0x14    /* sub client */
#define LC_SUB_LIBRARY  0x15    /* sub library */
#define LC_TWOLEVEL_HINTS 0x16  /* two-level namespace lookup hints */
#define LC_PREBIND_CKSUM  0x17  /* prebind checksum */

#define MH_NOUNDEFS     0x1             /* the object file has no undefined
                                           references */
#define MH_INCRLINK     0x2             /* the object file is the output of an
                                           incremental link against a base file
                                           and can't be link edited again */
#define MH_DYLDLINK     0x4             /* the object file is input for the
                                           dynamic linker and can't be staticly
                                           link edited again */
#define MH_BINDATLOAD   0x8             /* the object file's undefined
                                           references are bound by the dynamic
                                           linker when loaded. */
#define MH_PREBOUND     0x10            /* the file has its dynamic undefined
                                           references prebound. */
#define MH_SPLIT_SEGS   0x20            /* the file has its read-only and
                                           read-write segments split */
#define MH_LAZY_INIT    0x40            /* the shared library init routine is
                                           to be run lazily via catching memory
                                           faults to its writeable segments
                                           (obsolete) */
#define MH_TWOLEVEL     0x80            /* the image is using two-level name
                                           space bindings */
#define MH_FORCE_FLAT   0x100           /* the executable is forcing all images
                                           to use flat name space bindings */
#define MH_NOMULTIDEFS  0x200           /* this umbrella guarantees no multiple
                                           defintions of symbols in its
                                           sub-images so the two-level namespace
                                           hints can always be used. */
#define MH_NOFIXPREBINDING 0x400        /* do not have dyld notify the
                                           prebinding agent about this
                                           executable */
#define MH_PREBINDABLE  0x800           /* the binary is not prebound but can
                                           have its prebinding redone. only used
                                           when MH_PREBOUND is not set. */
#define MH_ALLMODSBOUND 0x1000          /* indicates that this binary binds to
                                           all two-level namespace modules of
                                           its dependent libraries. only used
                                           when MH_PREBINDABLE and MH_TWOLEVEL
                                           are both set. */
#define MH_SUBSECTIONS_VIA_SYMBOLS 0x2000/* safe to divide up the sections into
                                            sub-sections via symbols for dead
                                            code stripping */
#define MH_CANONICAL    0x4000          /* the binary has been canonicalized
                                           via the unprebind operation */
#define MH_WEAK_DEFINES 0x8000          /* the final linked image contains
                                           external weak symbols */
#define MH_BINDS_TO_WEAK 0x10000        /* the final linked image uses
                                           weak symbols */
#define MH_ALLOW_STACK_EXECUTION 0x20000/* When this bit is set, all stacks
                                           in the task will be given stack
                                           execution privilege.  Only used in
                                           MH_EXECUTE filetypes. */

#define MH_OBJECT       0x1             /* relocatable object file */
#define MH_EXECUTE      0x2             /* demand paged executable file */
#define MH_FVMLIB       0x3             /* fixed VM shared library file */
#define MH_CORE         0x4             /* core file */
#define MH_PRELOAD      0x5             /* preloaded executable file */
#define MH_DYLIB        0x6             /* dynamically bound shared library */
#define MH_DYLINKER     0x7             /* dynamic link editor */
#define MH_BUNDLE       0x8             /* dynamically bound bundle file */
#define MH_DYLIB_STUB   0x9             /* shared library stub for static */
                                        /*  linking only, no section contents */

/* Constants for the flags field of the mach_header */
#define MH_NOUNDEFS     0x1             /* the object file has no undefined
                                           references */
#define MH_INCRLINK     0x2             /* the object file is the output of an
                                           incremental link against a base file
                                           and can't be link edited again */
#define MH_DYLDLINK     0x4             /* the object file is input for the
                                           dynamic linker and can't be staticly
                                           link edited again */
#define MH_BINDATLOAD   0x8             /* the object file's undefined
                                           references are bound by the dynamic
                                           linker when loaded. */
#define MH_PREBOUND     0x10            /* the file has its dynamic undefined
                                           references prebound. */
#define MH_SPLIT_SEGS   0x20            /* the file has its read-only and
                                           read-write segments split */
#define MH_LAZY_INIT    0x40            /* the shared library init routine is
                                           to be run lazily via catching memory
                                           faults to its writeable segments
                                           (obsolete) */
#define MH_TWOLEVEL     0x80            /* the image is using two-level name
                                           space bindings */
#define MH_FORCE_FLAT   0x100           /* the executable is forcing all images
                                           to use flat name space bindings */
#define MH_NOMULTIDEFS  0x200           /* this umbrella guarantees no multiple
                                           defintions of symbols in its
                                           sub-images so the two-level namespace
                                           hints can always be used. */
#define MH_NOFIXPREBINDING 0x400        /* do not have dyld notify the
                                           prebinding agent about this
                                           executable */
#define MH_PREBINDABLE  0x800           /* the binary is not prebound but can
                                           have its prebinding redone. only used
                                           when MH_PREBOUND is not set. */
#define MH_ALLMODSBOUND 0x1000          /* indicates that this binary binds to
                                           all two-level namespace modules of
                                           its dependent libraries. only used
                                           when MH_PREBINDABLE and MH_TWOLEVEL*/
#define MH_SUBSECTIONS_VIA_SYMBOLS 0x2000 /* safe to divide up the sections into
                                            sub-sections via symbols for dead
                                            code stripping */
#define MH_CANONICAL    0x4000          /* the binary has been canonicalized
                                           via the unprebind operation */
#define MH_WEAK_DEFINES 0x8000          /* the final linked image contains
                                           external weak symbols */
#define MH_BINDS_TO_WEAK 0x10000        /* the final linked image uses
                                           weak symbols */

#define MH_ALLOW_STACK_EXECUTION 0x20000/* When this bit is set, all stacks
                                           in the task will be given stack
                                           execution privilege.  Only used in
                                           MH_EXECUTE filetypes. */

/* Constant for the magic field of the mach_header (32-bit architectures) */
#define MH_MAGIC        0xfeedface      /* the mach magic number */
#define MH_CIGAM        0xcefaedfe      /* NXSwapInt(MH_MAGIC) */


#define CPU_ARCH_MASK   0xff000000              /* mask for architecture bits */
#define CPU_ARCH_ABI64  0x01000000              /* 64 bit ABI */

#define CPU_TYPE_ANY            ((cpu_type_t) -1)


#define CPU_TYPE_VAX            ((cpu_type_t) 1)
/* skip                         ((cpu_type_t) 2)        */
/* skip                         ((cpu_type_t) 3)        */
/* skip                         ((cpu_type_t) 4)        */
/* skip                         ((cpu_type_t) 5)        */
#define CPU_TYPE_MC680x0        ((cpu_type_t) 6)
#define CPU_TYPE_X86            ((cpu_type_t) 7)
#define CPU_TYPE_I386           CPU_TYPE_X86            /* compatibility */
/* skip CPU_TYPE_MIPS           ((cpu_type_t) 8)        */
/* skip                         ((cpu_type_t) 9)        */
#define CPU_TYPE_MC98000        ((cpu_type_t) 10)
#define CPU_TYPE_HPPA           ((cpu_type_t) 11)
/* skip CPU_TYPE_ARM            ((cpu_type_t) 12)       */
#define CPU_TYPE_MC88000        ((cpu_type_t) 13)
#define CPU_TYPE_SPARC          ((cpu_type_t) 14)
#define CPU_TYPE_I860           ((cpu_type_t) 15)
/* skip CPU_TYPE_ALPHA          ((cpu_type_t) 16)       */
/* skip                         ((cpu_type_t) 17)       */
#define CPU_TYPE_POWERPC                ((cpu_type_t) 18)
#define CPU_TYPE_POWERPC64              (CPU_TYPE_POWERPC | CPU_ARCH_ABI64)


#define CPU_SUBTYPE_POWERPC_ALL         ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_POWERPC_601         ((cpu_subtype_t) 1)
#define CPU_SUBTYPE_POWERPC_602         ((cpu_subtype_t) 2)
#define CPU_SUBTYPE_POWERPC_603         ((cpu_subtype_t) 3)
#define CPU_SUBTYPE_POWERPC_603e        ((cpu_subtype_t) 4)
#define CPU_SUBTYPE_POWERPC_603ev       ((cpu_subtype_t) 5)
#define CPU_SUBTYPE_POWERPC_604         ((cpu_subtype_t) 6)
#define CPU_SUBTYPE_POWERPC_604e        ((cpu_subtype_t) 7)
#define CPU_SUBTYPE_POWERPC_620         ((cpu_subtype_t) 8)
#define CPU_SUBTYPE_POWERPC_750         ((cpu_subtype_t) 9)
#define CPU_SUBTYPE_POWERPC_7400        ((cpu_subtype_t) 10)
#define CPU_SUBTYPE_POWERPC_7450        ((cpu_subtype_t) 11)
#ifndef _OPEN_SOURCE_
#define CPU_SUBTYPE_POWERPC_Max         ((cpu_subtype_t) 10)
#define CPU_SUBTYPE_POWERPC_SCVger      ((cpu_subtype_t) 11)
#endif
#define CPU_SUBTYPE_POWERPC_970         ((cpu_subtype_t) 100)


typedef int             integer_t;
typedef unsigned int    natural_t;
typedef integer_t       cpu_type_t;
typedef integer_t       cpu_subtype_t;
typedef int             vm_prot_t;
typedef natural_t       mach_msg_type_size_t;
typedef natural_t       mach_msg_type_number_t;

/* 
 * The 32-bit mach header appears at the very beginning of the object file for
 * 32-bit architectures.
 */
struct mach_header {
        uint32_t        magic;          /* mach magic number identifier */
        cpu_type_t      cputype;        /* cpu specifier */
        cpu_subtype_t   cpusubtype;     /* machine specifier */
        uint32_t        filetype;       /* type of file */
        uint32_t        ncmds;          /* number of load commands */
        uint32_t        sizeofcmds;     /* the size of all the load commands */
        uint32_t        flags;          /* flags */
};


/*
 * The 64-bit mach header appears at the very beginning of object files for
 * 64-bit architectures.
 */
struct mach_header_64 {
        uint32_t        magic;          /* mach magic number identifier */
        cpu_type_t      cputype;        /* cpu specifier */
        cpu_subtype_t   cpusubtype;     /* machine specifier */
        uint32_t        filetype;       /* type of file */
        uint32_t        ncmds;          /* number of load commands */
        uint32_t        sizeofcmds;     /* the size of all the load commands */
        uint32_t        flags;          /* flags */
        uint32_t        reserved;       /* reserved */
};


/*
 * The load commands directly follow the mach_header.  The total size of all
 * of the commands is given by the sizeofcmds field in the mach_header.  All
 * load commands must have as their first two fields cmd and cmdsize.  The cmd
 * field is filled in with a constant for that command type.  Each command type
 * has a structure specifically for it.  The cmdsize field is the size in bytes
 * of the particular load command structure plus anything that follows it that
 * is a part of the load command (i.e. section structures, strings, etc.).  To
 * advance to the next load command the cmdsize can be added to the offset or
 * pointer of the current load command.  The cmdsize for 32-bit architectures
 * MUST be a multiple of 4 bytes and for 64-bit architectures MUST be a multiple
 * of 8 bytes (these are forever the maximum alignment of any load commands).
 * The padded bytes must be zero.  All tables in the object file must also
 * follow these rules so the file can be memory mapped.  Otherwise the pointers
 * to these tables will not work well or at all on some machines.  With all
 * padding zeroed like objects will compare byte for byte.
 */
struct load_command {
        uint32_t cmd;           /* type of load command */
        uint32_t cmdsize;       /* total size of command in bytes */
};


/*
 * Thread commands contain machine-specific data structures suitable for
 * use in the thread state primitives.  The machine specific data structures
 * follow the struct thread_command as follows.
 * Each flavor of machine specific data structure is preceded by an unsigned
 * long constant for the flavor of that data structure, an uint32_t
 * that is the count of longs of the size of the state data structure and then
 * the state data structure follows.  This triple may be repeated for many
 * flavors.  The constants for the flavors, counts and state data structure
 * definitions are expected to be in the header file <machine/thread_status.h>.
 * These machine specific data structures sizes must be multiples of
 * 4 bytes  The cmdsize reflects the total size of the thread_command
 * and all of the sizes of the constants for the flavors, counts and state
 * data structures.
 *
 * For executable objects that are unix processes there will be one
 * thread_command (cmd == LC_UNIXTHREAD) created for it by the link-editor.
 * This is the same as a LC_THREAD, except that a stack is automatically
 * created (based on the shell's limit for the stack size).  Command arguments
 * and environment variables are copied onto that stack.
 */
struct thread_command {
        uint32_t        cmd;            /* LC_THREAD or  LC_UNIXTHREAD */
        uint32_t        cmdsize;        /* total size of this command */
        /* uint32_t flavor                 flavor of thread state */
        /* uint32_t count                  count of longs in thread state */
        /* struct XXX_thread_state state   thread state for this flavor */
        /* ... */
};


/*
 * The segment load command indicates that a part of this file is to be
 * mapped into the task's address space.  The size of this segment in memory,
 * vmsize, maybe equal to or larger than the amount to map from this file,
 * filesize.  The file is mapped starting at fileoff to the beginning of
 * the segment in memory, vmaddr.  The rest of the memory of the segment,
 * if any, is allocated zero fill on demand.  The segment's maximum virtual
 * memory protection and initial virtual memory protection are specified
 * by the maxprot and initprot fields.  If the segment has sections then the
 * section structures directly follow the segment command and their size is
 * reflected in cmdsize.
 */
struct segment_command { /* for 32-bit architectures */
        uint32_t        cmd;            /* LC_SEGMENT */
        uint32_t        cmdsize;        /* includes sizeof section structs */
        char            segname[16];    /* segment name */
        uint32_t        vmaddr;         /* memory address of this segment */
        uint32_t        vmsize;         /* memory size of this segment */
        uint32_t        fileoff;        /* file offset of this segment */
        uint32_t        filesize;       /* amount to map from the file */
        vm_prot_t       maxprot;        /* maximum VM protection */
        vm_prot_t       initprot;       /* initial VM protection */
        uint32_t        nsects;         /* number of sections in segment */
        uint32_t        flags;          /* flags */
};


/*
 * The 64-bit segment load command indicates that a part of this file is to be
 * mapped into a 64-bit task's address space.  If the 64-bit segment has
 * sections then section_64 structures directly follow the 64-bit segment
 * command and their size is reflected in cmdsize.
 */
struct segment_command_64 { /* for 64-bit architectures */
        uint32_t        cmd;            /* LC_SEGMENT_64 */
        uint32_t        cmdsize;        /* includes sizeof section_64 structs */
        char            segname[16];    /* segment name */
        uint64_t        vmaddr;         /* memory address of this segment */
        uint64_t        vmsize;         /* memory size of this segment */
        uint64_t        fileoff;        /* file offset of this segment */
        uint64_t        filesize;       /* amount to map from the file */
        vm_prot_t       maxprot;        /* maximum VM protection */
        vm_prot_t       initprot;       /* initial VM protection */
        uint32_t        nsects;         /* number of sections in segment */
        uint32_t        flags;          /* flags */
};


struct ppc_thread_state
{
        unsigned int srr0;      /* Instruction address register (PC) */
        unsigned int srr1;      /* Machine state register (supervisor) */
        unsigned int r0;
        unsigned int r1;
        unsigned int r2;
        unsigned int r3;
        unsigned int r4;
        unsigned int r5;
        unsigned int r6;
        unsigned int r7;
        unsigned int r8;
        unsigned int r9;
        unsigned int r10;
        unsigned int r11;
        unsigned int r12;
        unsigned int r13;
        unsigned int r14;
        unsigned int r15;
        unsigned int r16;
        unsigned int r17;
        unsigned int r18;
        unsigned int r19;
        unsigned int r20;
        unsigned int r21;
        unsigned int r22;
        unsigned int r23;
        unsigned int r24;
        unsigned int r25;
        unsigned int r26;
        unsigned int r27;
        unsigned int r28;
        unsigned int r29;
        unsigned int r30;
        unsigned int r31;

        unsigned int cr;        /* Condition register */
        unsigned int xer;       /* User's integer exception register */
        unsigned int lr;        /* Link register */
        unsigned int ctr;       /* Count register */
        unsigned int mq;        /* MQ register (601 only) */

        unsigned int vrsave;    /* Vector Save Register */
};

typedef struct ppc_thread_state ppc_thread_state_t;


/*
 * A segment is made up of zero or more sections.  Non-MH_OBJECT files have
 * all of their segments with the proper sections in each, and padded to the
 * specified segment alignment when produced by the link editor.  The first
 * segment of a MH_EXECUTE and MH_FVMLIB format file contains the mach_header
 * and load commands of the object file before its first section.  The zero
 * fill sections are always last in their segment (in all formats).  This
 * allows the zeroed segment padding to be mapped into memory where zero fill
 * sections might be. The gigabyte zero fill sections, those with the section
 * type S_GB_ZEROFILL, can only be in a segment with sections of this type.
 * These segments are then placed after all other segments.
 *
 * The MH_OBJECT format has all of its sections in one segment for
 * compactness.  There is no padding to a specified segment boundary and the
 * mach_header and load commands are not part of the segment.
 *
 * Sections with the same section name, sectname, going into the same segment,
 * segname, are combined by the link editor.  The resulting section is aligned
 * to the maximum alignment of the combined sections and is the new section's
 * alignment.  The combined sections are aligned to their original alignment in
 * the combined section.  Any padded bytes to get the specified alignment are
 * zeroed.
 *
 * The format of the relocation entries referenced by the reloff and nreloc
 * fields of the section structure for mach object files is described in the
 * header file <reloc.h>.
 */
struct section { /* for 32-bit architectures */
        char            sectname[16];   /* name of this section */
        char            segname[16];    /* segment this section goes in */
        uint32_t        addr;           /* memory address of this section */
        uint32_t        size;           /* size in bytes of this section */
        uint32_t        offset;         /* file offset of this section */
        uint32_t        align;          /* section alignment (power of 2) */
        uint32_t        reloff;         /* file offset of relocation entries */
        uint32_t        nreloc;         /* number of relocation entries */
        uint32_t        flags;          /* flags (section type and attributes)*/
        uint32_t        reserved1;      /* reserved (for offset or index) */
        uint32_t        reserved2;      /* reserved (for count or sizeof) */
};

struct section_64 { /* for 64-bit architectures */
        char            sectname[16];   /* name of this section */
        char            segname[16];    /* segment this section goes in */
        uint64_t        addr;           /* memory address of this section */
        uint64_t        size;           /* size in bytes of this section */
        uint32_t        offset;         /* file offset of this section */
        uint32_t        align;          /* section alignment (power of 2) */
        uint32_t        reloff;         /* file offset of relocation entries */
        uint32_t        nreloc;         /* number of relocation entries */
        uint32_t        flags;          /* flags (section type and attributes)*/
        uint32_t        reserved1;      /* reserved (for offset or index) */
        uint32_t        reserved2;      /* reserved (for count or sizeof) */
        uint32_t        reserved3;      /* reserved */
};


#endif

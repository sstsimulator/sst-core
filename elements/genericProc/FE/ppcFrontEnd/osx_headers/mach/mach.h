#ifndef __MACH_H__
#define __MACH_H__

#include <mach-o/loader.h>


typedef integer_t       host_flavor_t;
#define HOST_BASIC_INFO         1       /* basic info */
#define HOST_SCHED_INFO         3       /* scheduling info */
#define HOST_RESOURCE_SIZES     4       /* kernel struct sizes */
#define HOST_PRIORITY_INFO      5       /* priority information */
#define HOST_SEMAPHORE_TRAPS    7       /* Has semaphore traps */
#define HOST_MACH_MSG_TRAP      8       /* Has mach_msg_trap */


typedef natural_t        mach_port_name_t;
typedef mach_port_name_t mach_port_t;
typedef unsigned int     mach_msg_bits_t;
typedef natural_t        mach_msg_size_t;
typedef integer_t        mach_msg_id_t;
typedef int              kern_return_t;
typedef unsigned int     mach_msg_trailer_type_t;
typedef unsigned int     mach_msg_trailer_size_t;
typedef integer_t        cpu_threadtype_t;


struct host_basic_info {
        integer_t               max_cpus;               /* max number of CPUs possible */
        integer_t               avail_cpus;             /* number of CPUs now available */
        natural_t               memory_size;            /* size of memory in bytes, capped at 2 GB */
        cpu_type_t              cpu_type;               /* cpu type */
        cpu_subtype_t           cpu_subtype;            /* cpu subtype */
        cpu_threadtype_t        cpu_threadtype;         /* cpu threadtype */
        integer_t               physical_cpu;           /* number of physical CPUs now available */
        integer_t               physical_cpu_max;       /* max number of physical CPUs possible */
        integer_t               logical_cpu;            /* number of logical cpu now available */
        integer_t               logical_cpu_max;        /* max number of physical CPUs possible */
        uint64_t                max_mem;                /* actual size of physical memory */
};
typedef struct host_basic_info  host_basic_info_data_t;
typedef struct host_basic_info  *host_basic_info_t;

#define HOST_BASIC_INFO_COUNT ((mach_msg_type_number_t) \
                (sizeof(host_basic_info_data_t)/sizeof(integer_t)))

#define KERN_SUCCESS                    0


typedef struct
{
  mach_msg_bits_t       msgh_bits;
  mach_msg_size_t       msgh_size;
  mach_port_t           msgh_remote_port;
  mach_port_t           msgh_local_port;
  mach_msg_size_t       msgh_reserved;
  mach_msg_id_t         msgh_id;
} mach_msg_header_t;


typedef struct
{
  mach_msg_trailer_type_t       msgh_trailer_type;
  mach_msg_trailer_size_t       msgh_trailer_size;
} mach_msg_trailer_t;


typedef struct {
    unsigned char       mig_vers;
    unsigned char       if_vers;
    unsigned char       reserved1;
    unsigned char       mig_encoding;
    unsigned char       int_rep;
    unsigned char       char_rep;
    unsigned char       float_rep;
    unsigned char       reserved2;
} NDR_record_t;


#endif

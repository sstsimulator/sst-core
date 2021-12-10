AC_DEFUN([SST_ENABLE_PERF_TRACKING], [
  enable_debug_perf_tracking="no"
  AC_ARG_ENABLE([perf-tracking], [AS_HELP_STRING([--enable-perf-tracking],
  [Enables tracking of simulator performance and component runtime. Options include component clock handler runtimes, openMPI synchronization overhead, and event handling. Communication sizes and information are also tracked. Tracking resolution can be implemented in nanosecond resolution or microsecond depening on individual workload. Slight performance penalties associated with enabling.])])
   AS_IF([test "x$enable_perf_tracking" = "xyes" ], [enable_debug_perf_tracking="yes"]) 
   AS_IF([test "$enable_debug_perf_tracking" = "yes"], [AC_DEFINE([SST_PERFORMANCE_INSTRUMENTING], [1],
               [EXPERIMENTAL Required for all performance tracking. Enables file creation and final output])])
   AS_IF([test "$enable_debug_perf_tracking" = "yes"], [AC_DEFINE([SST_HIGH_RESOLUTION_CLOCK], [0],
               [EXPERIMENTAL Enables nanosecond resolution clock. Disable for gettimeofday microsecond resolution])])
   AS_IF([test "$enable_debug_perf_tracking" = "yes"], [AC_DEFINE([SST_RUNTIME_PROFILING], [1],
               [EXPERIMENTAL Tracks execution time for each rank.])])
   AS_IF([test "$enable_debug_perf_tracking" = "yes"], [AC_DEFINE([SST_CLOCK_PROFILING], [1],
               [EXPERIMENTAL Tracks clock handler execution time and counters])])
   AS_IF([test "$enable_debug_perf_tracking" = "yes"], [AC_DEFINE([SST_EVENT_PROFILING], [1],
               [EXPERIMENTAL Tracks event and communication time and counters])])
   AS_IF([test "$enable_debug_perf_tracking" = "yes"], [AC_DEFINE([SST_PERIODIC_PRINT], [0],
               [EXPERIMENTAL Periodically prints performance information to files])])
   AS_IF([test "$enable_debug_perf_tracking" = "yes"], [AC_DEFINE([SST_PERIODIC_PRINT_THRESHOLD], [10000],
               [EXPERIMENTAL Tune to affect how often files are written.])])
])


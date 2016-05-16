
AC_DEFUN([SST_ENABLE_DEBUG_EVENT_TRACKING], [
  enable_debug_event_tracking="no"

  AC_ARG_ENABLE([event-tracking], [AS_HELP_STRING([--enable-event-tracking],
      [Enables additional tracking information for events and activities to help find memory leaks in event processing.  This adds information about first and last component to access the event.  This is a debug mode and will add significant memory usage to runs.])])

  AS_IF([test "x$enable_event_tracking" = "xyes" ], [enable_debug_event_tracking="yes"])

  AS_IF([test "$enable_debug_event_tracking" = "yes"], [AC_DEFINE([__SST_DEBUG_EVENT_TRACKING__], [1],
              [Tracks extra information about events and activities.])])
])

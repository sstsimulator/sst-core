Profiling experimental, see the output of `sst -h`
```bash
Advanced Options - Profiling (EXPERIMENTAL)
   --enable-profiling=POINTS      (S) Enables default profiling for the specified points.  Argument is a semicolon separated list specifying the points to enable.
   --profiling-output=FILE        (S) Set output location for profiling data [stdout (default) or a filename]
```

Profiling is "supported" in the context that it is not going away and the SST team will respond to bug reports.  It is not necessarily in its final form and some aspects of it may change before it is finalized.

The profiling command line options parses on both semicolons (line 1073 of simulation.cc) and colons (line 1085 of simulation.cc as indicated below).  The semicolons separate the profilers that should be enabled, and the colons are used for some of the parameters within the profiler’s description.  It will first split on semi-colons, then on colons later.  Documentation is still forthcoming, which is one of the reasons it's still experimental.  We haven’t finalized the formatting so it may  change.  You can avoid the semicolons by just having multiple `--enable-profiling` commands. 

Because things are not yet finalized and complete, not all error detection has been fully coded up (thus the stubs).  The goal is to have things finalized no later than SST 14, but hopefully much earlier depending on how some of the other feature coding goes.  We have found the profiling to already be useful in its current state and we encourage people to use it and provide feedback, which we will incorporate into the final version.

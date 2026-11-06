SST 16.1 has been tested in the following environments.

| OS           | Compiler  | Python | OpenMPI |
|--------------|-----------|--------|---------|
| OSX 15       | xcode 16  | 3.10   | 4.1.6   |
| OSX 26       | xcode 26  | 3.10   | 4.1.4   |
| RHEL 9       | gcc 11.5  | 3.9    | 4.1.6   |
| Rocky 8      | gcc 8.5   | 3.9    | 4.1.4   |
| Rocky 8      | gcc 8.5   | 3.9    | 5.0.8   |
| Rocky 8      | gcc 8.5   | 3.9    | -       |
| Rocky 8      | clang 18  | 3.13   | -       |
| Rocky 9      | gcc 11.5  | 3.9    | 4.1.6   |
| Rocky 10     | gcc 3.12  | 14.3   | 5.0.2   |
| TOSS 4.8     | gcc 8.5   | 3.12   | 4.1.8   |
| Ubuntu 24.04 | gcc 13.3  | 3.12   | 4.1.6   |
| Ubuntu 26.04 | gcc 15.2  | 3.14   | 5.0.10* |

\* When using the OpenMPI 5.0.10 package on Ubuntu 26.04, SST 16.1 sometimes produces garbled output although it appears to simulate correctly otherwise.


Additional SST platform information can be found at:
http://sst-simulator.org/SSTPages/SSTMainDocumentation/

Note: SST is only supported on 64-bit platforms.

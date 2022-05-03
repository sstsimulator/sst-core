![SST](http://sst-simulator.org/img/sst-logo-small.png)

# Structural Simulation Toolkit (SST)

#### Copyright (c) 2009-2022, National Technology and Engineering Solutions of Sandia, LLC (NTESS)

---

The Structural Simulation Toolkit (SST) was developed to explore innovations in highly concurrent systems where the ISA, microarchitecture, and memory interact with the programming model and communications system. The package provides two novel capabilities. The first is a fully modular design that enables extensive exploration of an individual system parameter without the need for intrusive changes to the simulator. The second is a parallel simulation environment based on MPI. This provides a high level of performance and the ability to look at large systems. The framework has been successfully used to model concepts ranging from processing in memory to conventional processors connected by conventional network interfaces and running MPI.

---


### Getting Started

#### Building From Source

##### Centos/RHEL 7

```sh
sudo yum install gcc gcc-c++ python3 python3-devel make automake git libtool libtool-ltdl-devel openmpi openmpi-devel zlib-devel
mkdir sst-core && cd sst-core
git clone https://github.com/sstsimulator/sst-core.git sst-core-src
(cd sst-core-src && ./autogen.sh)
mkdir build && cd build
../sst-core-src/configure \
  MPICC=/usr/lib64/openmpi/bin/mpicc \
  MPICXX=/usr/lib64/openmpi/bin/mpic++ \
  --prefix=$PWD/../sst-core-install
make install 
```

##### Ubuntu 20.04

```sh
DEBIAN_FRONTEND=noninteractive sudo apt install openmpi-bin openmpi-common libtool libtool-bin autoconf python3 python3-dev automake build-essential git 
mkdir sst-core && cd sst-core
git clone https://github.com/sstsimulator/sst-core.git sst-core-src
(cd sst-core-src && ./autogen.sh)
mkdir build && cd build
../sst-core-src/configure --prefix=$PWD/../sst-core-install
make install 
```

#### Testing Your Install

``` sh
/path/to/sst-core/install/bin/sst-test-core
```


---

Visit [sst-simulator.org](http://sst-simulator.org) to learn more about SST.

See [Contributing](https://github.com/sstsimulator/sst-core/blob/devel/CONTRIBUTING.md) to learn how to contribute to SST.

See [LICENSE](https://github.com/sstsimulator/sst-core/blob/devel/LICENSE.md) for our license

# dss-sdk

dss-sdk is a sub-component of the larger project, [DSS](https://github.com/OpenMPDK/DSS).

## Major Components
- The target component is located under the [target](target) directory.
- The host component is located under the [host](host) directory.

## Build Scripts
The build scripts and README.md for them are located in the [scripts](scripts) directory.

## Dependencies

### Operating System
CentOS 7.8

### Packages
```bash
sudo yum install epel-release
sudo yum groupinstall "Development Tools"
sudo yum install bc boost-devel check cmake cmake3 CUnit-devel dejagnu dpkg elfutils-libelf-devel expect glibc-devel jemalloc-devel Judy-devel libaio-devel libcurl-devel libuuid-devel meson ncurses-devel numactl-devel openssl-devel pulseaudio-libs-devel python3 python3-devel python3-pip rdma-core-devel redhat-lsb ruby-devel snappy-devel tbb-devel wget zlib-devel
```

### Python Package
```bash
python3 -m pip install pybind11
```

### Ruby packages
```bash
gem install dotenv:2.7.6 cabin:0.9.0 arr-pm:0.0.11 ffi:1.12.2 rchardet:1.8.0 git:1.7.0 rexml:3.2.4 backports:3.21.0 clamp:1.0.1 mustache:0.99.8 stud:0.0.23 insist:1.0.0 pleaserun:0.0.32 fpm:1.13.1
```

Note: GCC version 5.1 is required to build dss-sdk. The build script for GCC can be found in the DSS repo, in the scripts directory (https://github.com/OpenMPDK/DSS/blob/master/scripts/build_gcc.sh).

## Contributing
We welcome any contributions to dss-sdk. Please fork the repository and submit a pull request with your proposed changes. Before submitting a pull request, please make sure that your changes pass the existing unit tests and that new unit tests have been added to cover any new functionality.

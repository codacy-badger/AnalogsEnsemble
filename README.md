# [PAnEn: Parallel Analog Ensemble](https://weiming-hu.github.io/AnalogsEnsemble/)

[![DOI](https://zenodo.org/badge/130093968.svg)](https://zenodo.org/badge/latestdoi/130093968)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/f4cf23c626034d92a3bef0ba169a218a)](https://www.codacy.com?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Weiming-Hu/AnalogsEnsemble&amp;utm_campaign=Badge_Grade)
[![Build Status](https://travis-ci.com/Weiming-Hu/AnalogsEnsemble.svg?token=yTGL4zEDtXKy9xWq1dsP&branch=master)](https://travis-ci.com/Weiming-Hu/AnalogsEnsemble)
[![codecov](https://codecov.io/gh/Weiming-Hu/AnalogsEnsemble/branch/master/graph/badge.svg?token=tcGGOTyHHk)](https://codecov.io/gh/Weiming-Hu/AnalogsEnsemble)
[![lifecycle](https://img.shields.io/badge/lifecycle-experimental-orange.svg)](https://www.tidyverse.org/lifecycle/#experimental)
[![Binder](https://mybinder.org/badge.svg)](https://mybinder.org/v2/gh/Weiming-Hu/AnalogsEnsemble/master)
[![dockeri.co](https://dockeri.co/image/weiminghu123/panen)](https://hub.docker.com/r/weiminghu123/panen)

<!-- vim-markdown-toc GitLab -->

* [About Parallel Analog Ensemble](#about-parallel-analog-ensemble)
* [Citation](#citation)
* [Requirement and Dependencies](#requirement-and-dependencies)
* [Installation](#installation)
    * [C++ PAnEn](#c-panen)
    * [RAnEn](#ranen)
            * [One-Line Solution](#one-line-solution)
            * [Solution for a Specific Version](#solution-for-a-specific-version)
    * [gribConverter](#gribconverter)
        * [Mac OS](#mac-os)
    * [CMake Parameter Look-Up Table](#cmake-parameter-look-up-table)
* [References](#references)
* [Known Issues](#known-issues)
* [Feedbacks](#feedbacks)

<!-- vim-markdown-toc -->

## About Parallel Analog Ensemble

Parallel Analog Ensemble (PAnEn) is a parallel implementation for the Analog Ensemble (AnEn) technique which generates uncertainty information for a deterministic predictive model. Analogs are generated by using the predictive model and the corresponding historical observations. An introduction to Analog Ensemble technique can be found in [this post](https://weiming-hu.github.io/AnalogsEnsemble/2018/12/14/AnEn-explained.html). It has been successfully applied to forecasting of several weather variables, for example, temperature, wind speed, and solar photovoltaic generation. Publications can be found in the [reference section](#references).

`PAnEn` is developed by the [Geoinformatics and Earth Observation Laboratory](http://geoinf.psu.edu/) at Penn State University. This website contains information for installing and using the `PAnEn` programs and libraries. [R](https://weiming-hu.github.io/AnalogsEnsemble/R/) and [C++](https://weiming-hu.github.io/AnalogsEnsemble/CXX/) documentation is provided. [Posts](https://weiming-hu.github.io/AnalogsEnsemble/posts/) on various topics are included, and are organized by [tags](https://weiming-hu.github.io/AnalogsEnsemble/tags.html). If you have any questions, please submit a ticket [here](https://github.com/Weiming-Hu/AnalogsEnsemble/issues).

This package contains several programs and libraries:

- __AnEn__: This is the main C++ library. It provides the main functionality of the AnEn technique.
- __AnEnIO__: This is the file I/O library. Currently, it supports reading and writing [standard NetCDF](https://www.unidata.ucar.edu/software/netcdf/).
- __RAnEn__: This is the R interface to the `AnEn` library.
- __Apps__: Multiple executables in [the apps folder](https://github.com/Weiming-Hu/AnalogsEnsemble/tree/master/apps) are designed for analog computation and file management.

## Citation

Please consider citing this package. The `bibtex` entry can be found [here](https://github.com/Weiming-Hu/AnalogsEnsemble/blob/master/RAnalogs/RAnEn/inst/CITATION). If you are using `RAnEn`, you can also see the citation by typing `citation('RAnEn')`. Or you can use the following formatted text to cite this package.

```
Weiming Hu, Guido Cervone, Laura Clemente-Harding, and Martina Calovi. (2019). Parallel Analog Ensemble. Zenodo. http://doi.org/10.5281/zenodo.3384321
```

## Requirement and Dependencies

A list of dependencies are provided below. Note that you don't necessarily have to install them all because some of them can be automatically installed or you simply are not installing some components of the program. For example, you won't need R if you only need the C++ program, and you won't need Boost C++ and `NetCDF-C` if you are only installing the R package `RAnEn`.

| Dependency |                                            Description                                           |
|:----------:|:------------------------------------------------------------------------------------------------:|
|    CMake   |                                   Required for the C++ program.                                  |
|  GCC/Clang |                                   Required for the C++ program.                                  |
|  NetCDF-C  |        Optional for the C++ program. If it is not found, the project will try to build it.       |
|  Boost C++ | Optional for the C++ program. It is recommended to let the project build it for the C++ program. |
|   CppUnit  |                         Required for the C++ program when building tests.                        |
|      R     |                                    Required for the R library.                                   |
|   OpenMP   |                                   Optional for both R and C++.                                   |

## Installation

### C++ PAnEn

First, make sure you have already installed the dependencies. Typically, GNU compilers with a version later than `4.9`, `netCDF-C`, and `CMake` are required. If you are using `MacOS`, you probably need to install GNU compilers in order to have `OpenMP` multithreading available.

Then, please clone or download the repository [here](https://github.com/Weiming-Hu/AnalogsEnsemble) and create a `build/` folder under the repository directory.

```
git clone https://github.com/Weiming-Hu/AnalogsEnsemble.git
cd AnalogsEnsemble
mkdir build
cd build
cmake ..

# If you would like to change the default compiler, specify the compilers like this
#
# CC=[full path to CC] CXX=[full path to CXX] cmake ..

# You can scoll down to explore more parameters for cmake
#
# cmake -DCMAKE_INSTALL_PREFIX=/some/folder/ -DBOOST_TYPE=SYSTEM [other parameters] ..
```

Read the output messages and make sure there are no errors. If you would like to change `cmake` parameters, please delete all files in the `build/` folder and rerun the `cmake` command.

Then, please compile the programs and libraries.

```
make

# Or if you are using UNIX system, use the flag -j[number of cores] to parallelize the make process
#
# make -j4

# Build document if needed. The /html and the /man folders will be in your build directory
#
# make document

# If you want to install the program to your machine
#
# make install
```

After compilation, the programs and libraries should be in the folder `AnalogsEnsemble/output`. Please `cd` into the binary folder `[Where your repository folder is]/AnalogsEnsemble/output/bin/` and run the following command to see help messages.

```
./analogGenerator 
# Analog Ensemble program --- Analog Generator
# Available options:
#  -h [ --help ]             Print help information for options.
#  -c [ --config ] arg       Set the configuration file path. Command line 
#                            options overwrite options in configuration file. 
# ... [subsequent texts ignored]
```

If you want to clean up the folder, please do the following.

```
make clean
```

### RAnEn

_The command is the same for `RAnEn` installation and update._

The R version should be later than or equal to `3.3.0`. And the following R packages are needed:

- [BH](https://cran.r-project.org/web/packages/BH/index.html): `install.packages('Rcpp')`
- [Rcpp](https://cran.r-project.org/web/packages/Rcpp/index.html): `install.packages('BH')`

If your operating system is `Windows`, please also install [Rtools](https://cran.r-project.org/bin/windows/Rtools/).

##### One-Line Solution

The following R command installs the latest version of `RAnEn`.

```
# Read more if OpenMP is not supported
#
install.packages("https://github.com/Weiming-Hu/AnalogsEnsemble/raw/master/RAnalogs/releases/RAnEn_latest.tar.gz", repos = NULL)
```

##### Solution for a Specific Version

If you want to install a specific version of `RAnEn`, you can go to the [release folder](https://github.com/Weiming-Hu/AnalogsEnsemble/tree/master/RAnalogs/releases), copy the full name of the tarball file, replace the following part `[tarball name]` (including the square bracket) with it, and run the command in R.

```
# Read more if OpenMP is not supported
#
install.packages("https://github.com/Weiming-Hu/AnalogsEnsemble/raw/master/RAnalogs/releases/[tarball name]", repos = NULL)
```

**If `Openmp` multithreading is not supported**, or if you simply want to use a different compiler, please create a `Makevars` file under `~/.R`, with the following content.

```
CXX11=[C++11 compiler]
```

### gribConverter

`gribConverter` provides the functionality to convert from a GRIB2 file format to NetCDF file format directly that is ready to be used by other Analog computation tools like `similarityCalculator`. By default, this tool is **NOT** built.

If you have already installed `Eccodes` on your system, to build `gribConverter`, you need to include the following parameters. This would be recommended way of installation. The order of the parameters does not matter.

```
cmake -DBUILD_GRIBCONVERTER=ON <other arguments if you have any> ..

# If the eccodes library cannot be found at default locations,
# explicitly specify the root folder which contains folders like
# bin/, include/, and lib/.
#
cmake -DBUILD_GRIBCONVERTER=ON -CMAKE_PREFIX_PATH=<path to eccodes> <other arguments if you have any> ..
```

If you do not have `Eccodes` installed or you do not have the permission to install any libraries, the installation process is included in the `cmake` process. It will try to download and build the library for you under the project directory.

```
cmake -DBUILD_GRIBCONVERTER=ON -DECCODES_TYPE=BUILD <other arguments if you have any> ..
```

To build `Eccodes`, the following packages are needed:

- [gfortran](https://gcc.gnu.org/wiki/GFortran) is part of GNU
- [JPEG] on [Mac OS](https://formulae.brew.sh/formula/jpeg) or [Linux](https://www.howtoinstall.co/en/ubuntu/trusty/libjpeg-dev)
- At least one of the following:
  - OpenJPEG on [Mac OS](http://macappstore.org/openjpeg/) or [Linux](https://www.howtoinstall.co/en/ubuntu/xenial/libopenjpeg-dev)
  - Jasper on [Mac OS](http://macappstore.org/jasper/) or [Linux](https://www.howtoinstall.co/en/ubuntu/trusty/libjasper-dev)

**Note** that if you do not have either `OpenJPEG` or `Jasper`, the compilation would complete but the test would not be complete.

#### Mac OS

If you are using Mac OS, the recommended way to build `gribConverter` is to install `Eccodes` and then just link to it.

If you cannot install `Eccodes`, additional to that you need to make sure you have the required dependencies, you need check the followings:

```
# Check whether you have installed jasper.
```

Then you can follow the same commands for compiling and installation.

```
make <-j 4>
make test
```

After compilation, the programs and libraries should be in the folder `AnalogsEnsemble/output`. Please `cd` into the binary folder `[Where your repository folder is]/AnalogsEnsemble/output/bin/` and run the following command to see help messages.

```
~/github/AnalogsEnsemble/output/bin$ ./gribConverter 
Parallel Ensemble Forecasts --- GRIB Converter v 3.6.3
Copyright (c) 2018 Weiming Hu @ GEOlab
Available options:
  -h [ --help ]                 Print help information for options.
  -c [ --config ] arg           Set the configuration file path. Command line 
                                options overwrite options in configuration 
                                file.
  --output-type arg             Set the output type. Currently it supports 
                                'Forecasts' and 'Observations'.
  --folder arg                  Set the data folder.

# ... [subsequent texts ignored]
```

If you have specified an installation path, you program will reside under the `bin/` folder of your installation path.

For more information on how to use the tool, please see an example [here](https://weiming-hu.github.io/AnalogsEnsemble/2018/10/01/eccodes-gribconverter.html).

### CMake Parameter Look-Up Table

|      Parameter       |                                               Explanation                                                                                          |       Default      |
|:--------------------:|:--------------------------------------------------------------------------------------------------------------------------------------------------:|:------------------:|
|          CC          |                                         The C compiler to use.                                                                                     | [System dependent] |
|          CXX         |                                        The C++ compiler to use.                                                                                    | [System dependent] |
|     INSTALL\_RAnEn   |                                 Build and install the `RAnEn` library.                                                                             |         OFF        |
|      BOOST\_TYPE     | `BUILD` for building `Boost`; `SYSTEM` for using the library on the system.                                                                        |        BUILD       |
|  NETCDF\_CXX4\_TYPE  | `BUILD` for building `Netcdf C++4`; `SYSTEM` for using the library on the system.                                                                  |        BUILD       |
|     CPPUNIT\_TYPE    | `BUILD` for building `CppUnit`; `SYSTEM` for using the library on the system.                                                                      |        BUILD       |
|   CMAKE\_BUILD\_TYPE |                          `Release` for release mode; `Debug` for debug mode.                                                                       |       Release      |
|  CMAKE\_BUILD\_TESTS |                                             Build tests.                                                                                           |         ON         |
|  CMAKE\_PREFIX\_PATH |Which folder(s) should cmake search for packages besides the default. Paths are surrounded by double quotes and separated with semicolons.          |       [Empty]      |
|CMAKE\_INSTALL\_PREFIX|                                             The installation directory.                                                                            | [System dependent] |
|     BUILD\_NETCDF    |                           Build `NetCDF` library regardless of its existence.                                                                      |         OFF        |
|     USE\_NCCONFIG    |    Use the `nc_config` program if found. This might cause problems if `NetCDF` is not properly setup.                                              |         OFF        |
|      BUILD\_HDF5     |                            Build `HDF5` library regardless of its existence.                                                                       |         OFF        |
|        VERBOSE       |                          Print detailed messages during the compiling process.                                                                     |         OFF        |
|    CODE\_PROFILING   |                                     Print time profiling information.                                                                              |         OFF        |
|     ENABLE\_MPI      |  Build the MPI version of the CAnEnIO library for parallel I/O. This is option is not recommended.                                                 |         OFF        |
| BUILD\_GRIBCONVERTER | Build the GRIB Converter program. [Eccodes](https://confluence.ecmwf.int/display/ECC) library is required.                                         |         OFF        |
|    ECCODES\_TYPE     | `BUILD` for building `Eccodes`; `SYSTEM` for using the library on the system.                                                                      |         BUILD      |

## References

- [Delle Monache, Luca, et al. "Probabilistic weather prediction with an analog ensemble." Monthly Weather Review 141.10 (2013): 3498-3516.](https://journals.ametsoc.org/doi/full/10.1175/MWR-D-12-00281.1)
- [Clemente-Harding, L. A Beginners Introduction to the Analog Ensemble Technique](https://ral.ucar.edu/sites/default/files/public/images/events/WISE_documentation_20170725_Final.pdf)
- [Cervone, Guido, et al. "Short-term photovoltaic power forecasting using Artificial Neural Networks and an Analog Ensemble." Renewable energy 108 (2017): 274-286.](https://www.sciencedirect.com/science/article/pii/S0960148117301386)
- [Junk, Constantin, et al. "Predictor-weighting strategies for probabilistic wind power forecasting with an analog ensemble." Meteorol. Z 24.4 (2015): 361-379.](https://www.researchgate.net/profile/Constantin_Junk/publication/274951815_Predictor-weighting_strategies_for_probabilistic_wind_power_forecasting_with_an_analog_ensemble/links/552cd5710cf29b22c9c47260.pdf)
- [Balasubramanian, Vivek, et al. "Harnessing the power of many: Extensible toolkit for scalable ensemble applications." 2018 IEEE International Parallel and Distributed Processing Symposium (IPDPS). IEEE, 2018.](https://ieeexplore.ieee.org/abstract/document/8425207)

## Known Issues

Please see known issues in [this post](https://weiming-hu.github.io/AnalogsEnsemble/2018/10/17/known-issues.html). If you could not find solutions to your issue, please submit an issue. Thank you.

## Feedbacks

We appreciate collaborations and feedbacks from users. Please contact the maintainer [Weiming Hu](https://weiming-hu.github.io) through [weiming@psu.edu](weiming@psu.edu) or submit tickets if you have any problems.

Thank you!

```
# "`-''-/").___..--''"`-._
#  (`6_ 6  )   `-.  (     ).`-.__.`)   WE ARE ...
#  (_Y_.)'  ._   )  `._ `. ``-..-'    PENN STATE!
#    _ ..`--'_..-_/  /--'_.' ,'
#  (il),-''  (li),'  ((!.-'
# 
# Authors: 
#     Weiming Hu <weiming@psu.edu>
#     Guido Cervone <cervone@psu.edu>
#     Laura Clemente-Harding <lec170@psu.edu>
#     Martina Calovi <mcalovi@psu.edu>
#
# Contributors: 
#     Luca Delle Monache
#         
# Geoinformatics and Earth Observation Laboratory (http://geolab.psu.edu)
# Department of Geography and Institute for CyberScience
# The Pennsylvania State University
```

#!/bin/bash
###################################################################################
# Author: Weiming Hu <weiming@psu.edu>                                            #
#         Geoinformatics and Earth Observation Laboratory (http://geolab.psu.edu) #
#         Department of Geography                                                 #
#         Institute for Computational and Data Science                            #
#         The Pennsylvania State University                                       #
###################################################################################
#
# This is the script to install Eccodes. You might need sudo to run this.
#
# This script is designed to be run on CI machines. But this can also be run manually
# to install eccodes. You might need sudo permission to run this script, especially the
# last command to install the library to your system.
#

# This is the version to download
export ECCODES_VERSION=2.16.0

# Create a folder for eccodes. This will be the folder for both the source files and
# the compiled libraries.
#
mkdir ~/eccodes
cd ~/eccodes

# Download the tarball file
wget "https://confluence.ecmwf.int/download/attachments/45757960/eccodes-$ECCODES_VERSION-Source.tar.gz?api=v2" -O eccodes.tar.gz

# Extract the tarball file
tar -xvzf eccodes.tar.gz

# Carry out out-of-tree build
cd eccodes-$ECCODES_VERSION-Source
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=~/eccodes -DENABLE_PYTHON=OFF -DENABLE_FORTRAN=OFF ..

# Build the project in parallel with the -j 16 option. 16 is the number of cores.
make -j 16

# Test installation
ctest

# Install to system. This is the place you might need sudo permission
make install

# Print out message
echo "
########################################################################################
#                                                                                      #
# Now, Eccodes have been installed to your user space at ~/eccodes.                    #
# If you want cmake to be able to find eccodes, you can adding the following arguments #
#                                                                                      #
#          cmake -DCMAKE-PREFIX_PATH=~/eccodes [your extra arguments] ..               #
#                                                                                      #
########################################################################################
"


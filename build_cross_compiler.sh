#!/bin/bash

# This script builds a custom cross compiler (a compiler that can build for other platforms)
# Taken from https://github.com/randomdude/gcc-cross-x86_64-elf/

set -x

BINUTILS_VERSION=2.31.1
GCC_VERSION=8.2.0

wget -q https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.gz
wget -q https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.gz

tar zxf binutils-${BINUTILS_VERSION}.tar.gz
tar zxf gcc-${GCC_VERSION}.tar.gz

cd binutils-${BINUTILS_VERSION}
mkdir build 
cd build
../configure --target=x86_64-elf --with-sysroot --disable-nls --disable-werror
make
make install

cd ../../gcc-${GCC_VERSION}
mkdir build
cd build
../configure --target=x86_64-elf --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc

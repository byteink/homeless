#!/bin/bash

# 获取glibc的版本号
#
# usage: ./glibc-version.sh

LD_SCRIPT=`gcc -print-file-name=libc.so`
LIBC_PATH=`awk '/GROUP/ {match($0, "[^ ]+libc.so\.[^ ]+");print substr($0, RSTART, RLENGTH)}' $LD_SCRIPT`
$LIBC_PATH

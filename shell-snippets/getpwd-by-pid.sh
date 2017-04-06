#!/bin/bash

# 获取某个进程的工作目录
#
# usage: ./getpwd-by-pid.sh {pid}

cat /proc/$1/environ | tr '\000' '\n' | grep -oP "(?<=^PWD=).*"

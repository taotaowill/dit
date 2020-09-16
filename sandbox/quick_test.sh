#!/bin/bash

# kill old processes
pkill -f ditd
pkill -f dit

# cp files
cp ../ditd ./
cp ../dit ./

# 2.start ditd
nohup ./ditd &>/dev/null &
./dit ls /127.0.0.1:8500/


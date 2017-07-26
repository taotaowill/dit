#!/bin/bash

# kill old processes
pkill -9 ins
pkill -9 ditd

# cp files
cp ../ditd ./
cp ../dit ./

# start ins
HOSTNAME=`hostname`
MEMBERS="$HOSTNAME:8868,$HOSTNAME:8869,$HOSTNAME:8890,$HOSTNAME:8891,$HOSTNAME:8892"
echo "--cluster_members=$MEMBERS" > ins.flag
nohup ./ins --flagfile=ins.flag --server_id=1 &> ins.log &
nohup ./ins --flagfile=ins.flag --server_id=2 &> ins.log &
nohup ./ins --flagfile=ins.flag --server_id=3 &> ins.log &
nohup ./ins --flagfile=ins.flag --server_id=4 &> ins.log &
nohup ./ins --flagfile=ins.flag --server_id=5 &> ins.log &

# 2.start ditd
echo "--nexus_addr=$MEMBERS" > dit.flag
nohup ./ditd &>/dev/null &


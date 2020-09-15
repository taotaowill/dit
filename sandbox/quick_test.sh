#!/bin/bash

# kill old processes
pkill -f ins
pkill -f ditd

# cp files
cp ../ditd ./
cp ../dit ./

# start ins
HOSTNAME=`hostname`
MEMBERS="$HOSTNAME:8868,$HOSTNAME:8869,$HOSTNAME:8890,$HOSTNAME:8891,$HOSTNAME:8892"
echo "--cluster_members=$MEMBERS" > ins.flag
nohup ./ins --flagfile=ins.flag --server_id=1 &> ins1.log &
nohup ./ins --flagfile=ins.flag --server_id=2 &> ins2.log &
nohup ./ins --flagfile=ins.flag --server_id=3 &> ins3.log &
nohup ./ins --flagfile=ins.flag --server_id=4 &> ins4.log &
nohup ./ins --flagfile=ins.flag --server_id=5 &> ins5.log &

# 2.start ditd
echo "--nexus_addr=$MEMBERS" > dit.flag
nohup ./ditd &>/dev/null &


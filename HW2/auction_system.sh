#!/bin/bash
host=$1
player=$2

if [ $host -lt 1 -o $host -gt 10 ];then
    echo "Host number is out of range."
    exit
fi;

if [ $player -lt 8 -o $player -gt 12 ];then
    echo "Player number is out of range."
    exit
fi;

# Step.1 Prepare FIFO file for host
declare -a key # index-random
declare -a invkey  # random-index
declare -a available # random-can be used
declare -a totalScore # id-score
mkfifo "fifo_0.tmp"

for ((fn=1 ; fn<=$host ; fn=fn+1));do
    mkfifo "fifo_$fn.tmp"
    key+=([$fn]=$RANDOM)
    invkey+=([(key[$fn])]=$fn)
    available+=([(key[$fn])]=1)
done;

for ((p=1 ; p<=$player ; p=p+1));do
    totalScore+=([$p]=0)
done;
avaNum=$host


# Step.2 Generate all the combination

for ((i=1 ; i<=$host ; i=i+1));do
    ./host $i ${key[$i]} 0 &
done;

for ((a=1 ; a<=$player ; a=a+1)); do
for ((b=$a+1 ; b<=$player ; b=b+1)); do
for ((c=$b+1 ; c<=$player ; c=c+1)); do
for ((d=$c+1 ; d<=$player ; d=d+1)); do
for ((e=$d+1 ; e<=$player ; e=e+1)); do
for ((f=$e+1 ; f<=$player ; f=f+1)); do
for ((g=$f+1 ; g<=$player ; g=g+1)); do
for ((h=$g+1 ; h<=$player ; h=h+1)); do
while [ $avaNum -lt 1 ]; do
    ((avaNum++))
    read hostkey < "fifo_0.tmp"
    for((p=0 ; p<8 ; p=p+1));do
        read pid rank < "fifo_0.tmp"
        (( totalScore[$pid]+=(8-rank) ))
    done;
    available[$hostkey]=1
done;
for k in ${key[@]}; do
    r=-1 # r is the fifo ready to write
    if [ ${available[$k]} -eq 1 ];then
        r=${invkey[$k]};
        available[$k]=0
        ((avaNum--))
        break
    fi;
done
echo "$a $b $c $d $e $f $g $h" > "fifo_$r.tmp"
done;done;done;done;done;done;done;done;

# clean the hosts
while [ $avaNum != $host ]; do
    ((avaNum++))
    read hostkey < "fifo_0.tmp"
    for((p=0 ; p<8 ; p=p+1));do
        read pid rank < "fifo_0.tmp"
        (( totalScore[$pid]+=(8-rank) ))
    done;
    available[$hostkey]=1
done;

# Step.3 Send an ending message
for ((i=1;i<=$host;i++));do
    echo "-1 -1 -1 -1 -1 -1 -1 -1" > "fifo_$i.tmp"
done;

# Step.4 Output
for ((p=1;p<=$player;p=p+1));do
    echo "$p ${totalScore[$p]}"
done;

# Step.5 Remove all the FIFO
for ((fn=0 ; fn<=$host; fn=fn+1));do
    rm "fifo_$fn.tmp"
done;

wait
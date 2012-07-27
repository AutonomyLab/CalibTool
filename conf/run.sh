#!/bin/bash

# add path to tracker binary
PATH=../build:$PATH

#CLI=testcli
#$CLI 0 > log0 &
#$CLI 1 > log1 &
#$CLI 2 > log2 &
#$CLI 3 > log3 &
#$CLI 4 > log4 &
#$CLI 5 > log5 &


TRACKER=tracker
TRACK=track.yaml
OPTS="-u -r"

$TRACKER -t $TRACK $OPTS -c lb-calib.yaml -v 0 &
sleep 1
$TRACKER -t $TRACK $OPTS -c lt-calib.yaml -v 1 &
sleep 1
$TRACKER -t $TRACK $OPTS -c mb-calib.yaml -v 2 &
sleep 1
$TRACKER -t $TRACK $OPTS -c mt-calib.yaml -v 3 &
sleep 1
$TRACKER -t $TRACK $OPTS -c rb-calib.yaml -v 4 &
sleep 1
$TRACKER -t $TRACK $OPTS -c rt-calib.yaml -v 5 &



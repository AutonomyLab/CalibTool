#!/bin/bash

# add path to tracker binary
PATH=../build:$PATH

TRACKER=tracker
TRACK=track.yaml
OPTS="-u"

$TRACKER -t $TRACK $OPTS -c lb-calib.yaml   -v 0 &
$TRACKER -t $TRACK $OPTS -c lt-calib.yaml    -v 1 &
$TRACKER -t $TRACK $OPTS -c mb-calib.yaml -v 2 &
$TRACKER -t $TRACK $OPTS -c mt-calib.yaml  -v 3 &

# need to plug in the last two cameras
#$TRACKER -t $TRACK $OPTS -c rb-calib.yaml   -v 4 &
#$TRACKER -t $TRACK $OPTS -c rt-calib.yaml    -v 5 &


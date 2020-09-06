#!/bin/bash -e

./libtool --mode=execute --  \
    gdb -ex 'handle SIGALRM noprint nostop nopass' --args $*

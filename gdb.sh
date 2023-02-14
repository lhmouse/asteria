#!/bin/bash -e

./libtool --mode=execute --  \
    gdb -iex 'handle SIGALRM noprint nostop nopass' --args $*

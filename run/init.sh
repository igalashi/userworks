#!/bin/sh

redis-server $NESTDAQ/etc/redis.conf --loadmodule $NESTDAQ/lib/redistimeseries.so
RIHOST=0.0.0.0 redisinsight-linux64 &
daq-webctl >& $NESTDAQ/log/daq-webctl.log &
#webgui-ws --engine http:8080 >& $NESTDAQ/log/webgui.log &

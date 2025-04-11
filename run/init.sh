#!/bin/sh

CTRLPORT=9090

redis-cli ping
if [ $? -ne 0 ]
then
	redis-server $NESTDAQ/etc/redis.conf --loadmodule $NESTDAQ/lib/redistimeseries.so
	echo 'Start Redis'
fi

wget -O /dev/null localhost:$CTRLPORT
if [ $? -ne 0 ]
then
        killall -KILL daq-webctl

        #daq-webctl >& /dev/null &
        #daq-webctl >& $NESTDAQ/log/daq-webctl.log &
        daq-webctl --http-uri http://0.0.0.0:$CTRLPORT >& /dev/null &

        echo 'ReStart DAQ Control'
fi

#redis-server $NESTDAQ/etc/redis.conf --loadmodule $NESTDAQ/lib/redistimeseries.so
##RIHOST=0.0.0.0 redisinsight-linux64 &
##daq-webctl >& /dev/null &
#daq-webctl --http-uri http://0.0.0.0:9090 >& /dev/null &
##daq-webctl >& $NESTDAQ/log/daq-webctl.log &
##webgui-ws --engine http:8080 >& $NESTDAQ/log/webgui.log &

#!/bin/sh

LFLAG=0
DFLAG=0
if test $# -gt 0
then
	if test $1 = "-l"
	then
		LFLAG=0
		DFLAG=1
	fi

	if test $1 = "-d"
	then
		LFLAG=1
		DFLAG=0
	fi
fi

#echo $LFLAG " " $DFLAG

COUNTS=1
IPHEAD="192.168.10."

#NUM=`seq 100 130`
NUM='17 29 30 31 45 46 47 63 64 68 81 82 99 100 116 117 118'

for i in $NUM
do
	ip=$IPHEAD$i
	out=`ping -c $COUNTS $ip`
	stat=$?
	if test $stat -eq 0
	then
		if test $LFLAG -eq 0
		then
			echo $ip "	: LIVE"
		fi
	else
		if test $DFLAG -eq 0
		then
			echo $ip "	: DEAD"
		fi
	fi
done


#192.168.10.1    : LIVE
#192.168.10.11   : LIVE
#192.168.10.12   : LIVE

#192.168.10.17   : LIVE
#192.168.10.29   : LIVE
#192.168.10.30   : LIVE
#192.168.10.31   : LIVE
#192.168.10.45   : LIVE
#192.168.10.46   : LIVE
#192.168.10.47   : LIVE
#192.168.10.63   : LIVE
#192.168.10.64   : LIVE
#192.168.10.68   : LIVE
#192.168.10.81   : LIVE
#192.168.10.82   : LIVE
#192.168.10.99   : LIVE
#192.168.10.100  : LIVE
#192.168.10.116  : LIVE
#192.168.10.117  : LIVE
#192.168.10.118  : LIVE

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

NUM=`seq 17 121`
#NUM='17 29 30 31 45 46 47 63 64 68 81 82 99 100 116 117 118'

for i in $NUM
do
	ip=$IPHEAD$i
	out=`ping -w 1000 -c $COUNTS $ip`
	stat=$?
	if test $stat -eq 0
	then
		if test $LFLAG -eq 0
		then
			echo $ip "	: ALIVE"
		fi
	else
		if test $DFLAG -eq 0
		then
			echo $ip "	: DEAD "
		fi
	fi
done

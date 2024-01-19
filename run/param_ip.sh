#!/bin/bash

server=redis://127.0.0.1:6379/2

function param () {
  # "instance":"field" "value"
  #echo redis-cli -u $server set parameters:$1:$2 ${@:3}
  #redis-cli -u $server set parameters:$1:$2 ${@:3}
  echo redis-cli -u $server hset parameters:$1 ${@:2}
  redis-cli -u $server hset parameters:$1 ${@:2}
}


#==============================================================================
#      isntance-id       field  value    field value   field          value
#==============================================================================
#param Sampler-0          text   Hello    rate  2       max-iterations 0
#param RecbeSampler-0     ip     192.168.10.17
#param Sink-0             multipart   false
#param Sink-0             multipart   true

IPLIST='ip_recbe.txt'
DEVNAME='RecbeSampler'

INDEX=0
for F in `cat $IPLIST`
do
	HEAD=${F:0:3}
	if [ "$HEAD" == "192" ]
	then
		NAME=$DEVNAME'-'$((INDEX++))
		#echo param $NAME 'ip' $F
		param $NAME' ip '$F
	fi
done

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
#param  Sampler-0         text   Hello    rate  2       max-iterations 0

param  RecbeSampler-0    ip     192.168.10.17
param  RecbeSampler-1    ip     192.168.10.29
param  RecbeSampler-2    ip     192.168.10.30
param  RecbeSampler-3    ip     192.168.10.31

#param STFBuilder-0     max-hbf	3
#param STFBuilder-1     max-hbf	3
#param STFBuilder-2     max-hbf	3
#param STFBuilder-3     max-hbf	3

#param fltcoin-0        data-supress true

#param Sink-0 multipart false
#param Sink-0 multipart true


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
param  RecbeSampler-4    ip     192.168.10.45
param  RecbeSampler-5    ip     192.168.10.46
param  RecbeSampler-6    ip     192.168.10.47
param  RecbeSampler-7    ip     192.168.10.63
param  RecbeSampler-8    ip     192.168.10.64
##param  RecbeSampler-9    ip     192.168.10.68
#param  RecbeSampler-10   ip     192.168.10.81
#param  RecbeSampler-11   ip     192.168.10.82
#param  RecbeSampler-12   ip     192.168.10.99
#param  RecbeSampler-13   ip     192.168.10.100
##param  RecbeSampler-14   ip     192.168.10.116
#param  RecbeSampler-15   ip     192.168.10.117
#param  RecbeSampler-16   ip     192.168.10.118
param  RecbeSampler-9    ip     192.168.10.81
param  RecbeSampler-10   ip     192.168.10.82
param  RecbeSampler-11   ip     192.168.10.99
param  RecbeSampler-12   ip     192.168.10.100
param  RecbeSampler-13   ip     192.168.10.117
param  RecbeSampler-14   ip     192.168.10.118


#param STFBuilder-0     max-hbf	3
#param STFBuilder-1     max-hbf	3
#param STFBuilder-2     max-hbf	3
#param STFBuilder-3     max-hbf	3

#param fltcoin-0        data-supress true

#param Sink-0 multipart false
#param Sink-0 multipart true


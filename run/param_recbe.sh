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

#param  RecbeSampler-0    ip     192.168.10.17
#param  RecbeSampler-1    ip     192.168.10.18
#param  RecbeSampler-2    ip     192.168.10.19
#param  RecbeSampler-3    ip     192.168.10.20
#param  RecbeSampler-4    ip     192.168.10.21
#param  RecbeSampler-5    ip     192.168.10.22
#param  RecbeSampler-6    ip     192.168.10.23
#param  RecbeSampler-7    ip     192.168.10.24
#param  RecbeSampler-8    ip     192.168.10.25
#param  RecbeSampler-9    ip     192.168.10.26
#param  RecbeSampler-10   ip     192.168.10.27
#param  RecbeSampler-11   ip     192.168.10.28
#param  RecbeSampler-11   ip     192.168.10.29
#param  RecbeSampler-11   ip     192.168.10.30
#param  RecbeSampler-12   ip     192.168.10.100
#param  RecbeSampler-13   ip     192.168.10.117
#param  RecbeSampler-14   ip     192.168.10.118

param  RecbeSampler-0    ip 192.168.10.17
param  RecbeSampler-1    ip 192.168.10.18
param  RecbeSampler-2    ip 192.168.10.19
param  RecbeSampler-3    ip 192.168.10.20
param  RecbeSampler-4    ip 192.168.10.21
param  RecbeSampler-5    ip 192.168.10.22
param  RecbeSampler-6    ip 192.168.10.23
param  RecbeSampler-7    ip 192.168.10.24
param  RecbeSampler-8    ip 192.168.10.25
param  RecbeSampler-9    ip 192.168.10.26
param  RecbeSampler-10   ip 192.168.10.27
param  RecbeSampler-11   ip 192.168.10.28
param  RecbeSampler-12   ip 192.168.10.29
param  RecbeSampler-13   ip 192.168.10.30
param  RecbeSampler-14   ip 192.168.10.31
param  RecbeSampler-15   ip 192.168.10.32
param  RecbeSampler-16   ip 192.168.10.33
param  RecbeSampler-17   ip 192.168.10.34
param  RecbeSampler-18   ip 192.168.10.35
param  RecbeSampler-19   ip 192.168.10.36
param  RecbeSampler-20   ip 192.168.10.37
param  RecbeSampler-21   ip 192.168.10.38
param  RecbeSampler-22   ip 192.168.10.39
param  RecbeSampler-23   ip 192.168.10.40
param  RecbeSampler-24   ip 192.168.10.41
param  RecbeSampler-25   ip 192.168.10.42
param  RecbeSampler-26   ip 192.168.10.43
param  RecbeSampler-27   ip 192.168.10.44
param  RecbeSampler-28   ip 192.168.10.45
param  RecbeSampler-29   ip 192.168.10.46
param  RecbeSampler-30   ip 192.168.10.47
param  RecbeSampler-31   ip 192.168.10.48
param  RecbeSampler-32   ip 192.168.10.49
param  RecbeSampler-33   ip 192.168.10.50
param  RecbeSampler-34   ip 192.168.10.51
param  RecbeSampler-35   ip 192.168.10.52
param  RecbeSampler-36   ip 192.168.10.53
param  RecbeSampler-37   ip 192.168.10.54
param  RecbeSampler-38   ip 192.168.10.55
param  RecbeSampler-39   ip 192.168.10.56
param  RecbeSampler-40   ip 192.168.10.57
param  RecbeSampler-41   ip 192.168.10.58
param  RecbeSampler-42   ip 192.168.10.59
param  RecbeSampler-43   ip 192.168.10.60
param  RecbeSampler-44   ip 192.168.10.61
param  RecbeSampler-45   ip 192.168.10.62
param  RecbeSampler-46   ip 192.168.10.63
param  RecbeSampler-47   ip 192.168.10.64
param  RecbeSampler-48   ip 192.168.10.65
param  RecbeSampler-49   ip 192.168.10.66
param  RecbeSampler-50   ip 192.168.10.67
param  RecbeSampler-51   ip 192.168.10.68
param  RecbeSampler-52   ip 192.168.10.69
param  RecbeSampler-53   ip 192.168.10.70
param  RecbeSampler-54   ip 192.168.10.71
param  RecbeSampler-55   ip 192.168.10.72
param  RecbeSampler-56   ip 192.168.10.73
param  RecbeSampler-57   ip 192.168.10.74
param  RecbeSampler-58   ip 192.168.10.75
param  RecbeSampler-59   ip 192.168.10.76
param  RecbeSampler-60   ip 192.168.10.77
param  RecbeSampler-61   ip 192.168.10.78
param  RecbeSampler-62   ip 192.168.10.79
param  RecbeSampler-63   ip 192.168.10.80
param  RecbeSampler-64   ip 192.168.10.81
param  RecbeSampler-65   ip 192.168.10.82
param  RecbeSampler-66   ip 192.168.10.83
param  RecbeSampler-67   ip 192.168.10.84
param  RecbeSampler-68   ip 192.168.10.85
param  RecbeSampler-69   ip 192.168.10.86
param  RecbeSampler-70   ip 192.168.10.87
param  RecbeSampler-71   ip 192.168.10.88
param  RecbeSampler-72   ip 192.168.10.89
param  RecbeSampler-73   ip 192.168.10.90
param  RecbeSampler-74   ip 192.168.10.91
param  RecbeSampler-75   ip 192.168.10.92
param  RecbeSampler-76   ip 192.168.10.93
param  RecbeSampler-77   ip 192.168.10.94
param  RecbeSampler-78   ip 192.168.10.95
param  RecbeSampler-79   ip 192.168.10.96
param  RecbeSampler-80   ip 192.168.10.97
param  RecbeSampler-81   ip 192.168.10.98
param  RecbeSampler-82   ip 192.168.10.99
param  RecbeSampler-83   ip 192.168.10.100
param  RecbeSampler-84   ip 192.168.10.101
param  RecbeSampler-85   ip 192.168.10.102
param  RecbeSampler-86   ip 192.168.10.103
param  RecbeSampler-87   ip 192.168.10.104
param  RecbeSampler-88   ip 192.168.10.105
param  RecbeSampler-89   ip 192.168.10.106
param  RecbeSampler-90   ip 192.168.10.107
param  RecbeSampler-91   ip 192.168.10.108
param  RecbeSampler-92   ip 192.168.10.109
param  RecbeSampler-93   ip 192.168.10.110
param  RecbeSampler-94   ip 192.168.10.111
param  RecbeSampler-95   ip 192.168.10.112
param  RecbeSampler-96   ip 192.168.10.113
param  RecbeSampler-97   ip 192.168.10.114
param  RecbeSampler-98   ip 192.168.10.115
param  RecbeSampler-99   ip 192.168.10.116
param  RecbeSampler-100  ip 192.168.10.117
param  RecbeSampler-101  ip 192.168.10.118
param  RecbeSampler-102  ip 192.168.10.119
param  RecbeSampler-103  ip 192.168.10.120
param  RecbeSampler-104  ip 192.168.10.121

#param Sink-0 multipart false
#param Sink-0 multipart true


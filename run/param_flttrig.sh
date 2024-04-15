#!/bin/bash

server=redis://127.0.0.1:6379/2

function param () {
  # "instance":"field" "value"
  #echo redis-cli -u $server set parameters:$1:$2 ${@:3}
  #redis-cli -u $server set parameters:$1:$2 ${@:3}
  echo redis-cli -u $server hset parameters:$1 ${@:2}
  redis-cli -u $server hset parameters:$1 ${@:2}
}

function param1 () {
  #echo redis-cli -u $server hset parameters:$1 ${@:2}
	echo zero:$0
	echo one:$1
	echo two:$2
	echo three:$3
  echo redis-cli -u $server hset parameters:$1 $3
  redis-cli -u $server hset parameters:$1 $2 $3
}


#==============================================================================
#      isntance-id       field  value    field value   field          value
#==============================================================================
#param  Sampler-0         text   Hello    rate  2       max-iterations 0

#SIG="'(0xc0a802a9 0 0) (0xc0a802a9 1 0) (0xc0a802a9 2 0) (0xc0a802a9 3 0) (0xc0a802a9 4 0) (0xc0a802a9 5 0) (0xc0a802aa 32 0) 0xc0a802aa 33 0) 0xc0a802aa 34 0)'"
#param1 flttrig trigger-signals "$SIG"

redis-cli -u $server hset parameters:flttrig trigger-signals "(0xc0a802a9 0 0) (0xc0a802a9 1 0) (0xc0a802a9 2 0) (0xc0a802a9 3 0) (0xc0a802a9 4 0) (0xc0a802a9 5 0) (0xc0a802aa 32 0) (0xc0a802aa 33 0) (0xc0a802aa 34 0) (0xc0a802aa 35 0)"
#redis-cli -u $server hset parameters:flttrig trigger-formula "RPN 0 1 & 2 3 & | 4 5 & | 6 7 & 8 9 & | &"
redis-cli -u $server hset parameters:flttrig trigger-formula "((0 & 1) | (2 & 3) | (4 & 5))  & ((6 & 7) | (8 & 9))"

#redis-cli -u $server hset parameters:LogicFilter trigger-signals "(0xc0a802a9 0 0) (0xc0a802a9 1 0) (0xc0a802a9 2 0) (0xc0a802a9 3 0) (0xc0a802a9 4 0) (0xc0a802a9 5 0) (0xc0a802aa 32 0) (0xc0a802aa 33 0) (0xc0a802aa 34 0) (0xc0a802aa 35 0)"
#redis-cli -u $server hset parameters:LogicFilter trigger-formula "RPN 0 1 & 2 3 & | 4 5 & | 6 7 & 8 9 & | &"
#redis-cli -u $server hset parameters:LogicFilter trigger-formula "((0 & 1) | (2 & 3) | (4 & 5))  & ((6 & 7) | (8 & 9))"


redis-cli -u $server hset parameters:LogicFilter trigger-signals "(0xc0a802a9 8 0) (0xc0a802a9 10 0) (0xc0a802aa 2 0) (0xc0a802aa 7 0)"
redis-cli -u $server hset parameters:LogicFilter trigger-formula "(0 & 1) | (2 & 3)"

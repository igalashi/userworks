#!/bin/sh

server=redis://127.0.0.1:6379/0
redis-cli -u $server KEYS 'daq_service:*' | xargs redis-cli -u $server DEL

#redis-cli KEYS 'daq_service:*' | xargs redis-cli DEL
#redis-cli -u redis://127.0.0.1:6379/0 flushall
#redis-cli -u redis://127.0.0.1:6379/1 flushall
#redis-cli -u redis://127.0.0.1:6379/2 flushall

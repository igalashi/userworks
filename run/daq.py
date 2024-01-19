#!/usr/bin/env python3

import redis

server = 'localhost'
port = 6379
db = 0

r = redis.StrictRedis(host = server, port = port, db = db)

val = r.get("daq_service:RecbeSampler:RecbeSampler-2:fair-mq-state");
r.publish()

print( val )




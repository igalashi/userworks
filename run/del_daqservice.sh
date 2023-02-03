#!/bin/sh
redis-cli KEYS 'daq_service:*' | xargs redis-cli DEL

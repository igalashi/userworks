#!/bin/sh
redis-cli KEY 'daq_service:*' | xargs redis-cli DEL

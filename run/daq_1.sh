#!/bin/bash

server=redis://127.0.0.1:6379/0

function daqctrl () {
redis-cli -u $server PUBLISH daqctl "{
    \"command\": \"change_state\",
    \"value\": \"$1\",
    \"services\": [
        \"all\"
    ],
    \"instances\": [
        \"all\"
    ]
}"
}

function c_reset_device () {
redis-cli PUBLISH daqctl "{
    \"command\": \"change_state\",
    \"value\": \"RESET DEVICE\",
    \"services\": [
        \"all\"
    ],
    \"instances\": [
        \"all\"
    ]
}"
}

function c_connect () {
redis-cli PUBLISH daqctl "{
    \"command\": \"change_state\",
    \"value\": \"CONNECT\",
    \"services\": [
        \"all\"
    ],
    \"instances\": [
        \"all\"
    ]
}"
}

function c_init () {
redis-cli PUBLISH daqctl "{
    \"command\": \"change_state\",
    \"value\": \"INIT TASK\",
    \"services\": [
        \"all\"
    ],
    \"instances\": [
        \"all\"
    ]
}"
}

function c_run () {
redis-cli PUBLISH daqctl "{
    \"command\": \"change_state\",
    \"value\": \"RUN\",
    \"services\": [
        \"all\"
    ],
    \"instances\": [
        \"all\"
    ]
}"
}

function c_stop () {
redis-cli PUBLISH daqctl "{
    \"command\": \"change_state\",
    \"value\": \"STOP\",
    \"services\": [
        \"all\"
    ],
    \"instances\": [
        \"all\"
    ]
}"
}

function c_reset_task () {
redis-cli PUBLISH daqctl "{
    \"command\": \"change_state\",
    \"value\": \"RESET TASK\",
    \"services\": [
        \"all\"
    ],
    \"instances\": [
        \"all\"
    ]
}"
}

daqctrl "$1"

#if test $1 == "RESET_DEVICE"
#then
#	KEY="RESET DEVICE"
#	echo hello
#else
#	KEY=$1
#fi
#	echo $KEY


#daqctrl "RESET DEVICE"
#daqctrl "CONNECT"
#sleep 1
#daqctrl "RESET DEVICE"

#c_reset_device
#c_connect
#sleep 3
#c_init
#sleep 0.2
#c_run
#sleep 5
#c_stop
#sleep 0.2
#c_reset_task
#sleep 0.2
#c_reset_device


# PUBLISH daqctl "{\n    \"command\": \"change_state\",\n    \"value\": \"CONNECT\",\n    \"services\": [\n        \"all\"\n    ],\n    \"instances\": [\n        \"all\"\n    ]\n}\n"
# PUBLISH daqctl "{\n    \"command\": \"change_state\",\n    \"value\": \"INIT TASK\",\n    \"services\": [\n        \"all\"\n    ],\n    \"instances\": [\n        \"all\"\n    ]\n}\n"
# PUBLISH daqctl "{\n    \"command\": \"change_state\",\n    \"value\": \"RUN\",\n    \"services\": [\n        \"all\"\n    ],\n    \"instances\": [\n        \"all\"\n    ]\n}\n"
# PUBLISH daqctl "{\n    \"command\": \"change_state\",\n    \"value\": \"STOP\",\n    \"services\": [\n        \"all\"\n    ],\n    \"instances\": [\n        \"all\"\n    ]\n}\n"
# PUBLISH daqctl "{\n    \"command\": \"change_state\",\n    \"value\": \"RESET TASK\",\n    \"services\": [\n        \"all\"\n    ],\n    \"instances\": [\n        \"all\"\n    ]\n}\n"
# PUBLISH daqctl "{\n    \"command\": \"change_state\",\n    \"value\": \"RESET DEVICE\",\n    \"services\": [\n        \"all\"\n    ],\n    \"instances\": [\n        \"all\"\n    ]\n}\n"


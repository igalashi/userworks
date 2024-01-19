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

#daqctrl "$1"

if test $1 == "reset_device"
then
	daqctrl "RESET DEVICE"
elif test $1 = "connect"
then
	daqctrl "CONNECT"
elif test $1 = "init"
then
	daqctrl "INIT TASK"
elif test $1 = "run"
then
	daqctrl "RUN"
elif test $1 = "stop"
then
	daqctrl "STOP"
elif test $1 = "reset_task"
then
	daqctrl "RESET TASK"
elif test $1 = "end"
then
	daqctrl "END"
fi


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


#!/bin/bash

function kill_fairmq_devices (){
    for fairmq_dev in AmQStrTdcSampler STFBuilder Scaler TimeFrameBuilder FileSink
    do
	echo killing ${fairmq_dev}...
	while true
	do
	    ret=`ps aux | grep start_device | grep $fairmq_dev | grep -v grep | head -1`
	    ret=`echo $ret | awk '{print $2}'`
	    if [ ! "$ret" = ""  ]; then
		echo $ret
		echo kill -KILL $ret
		kill -KILL $ret
	    else
		break
	    fi
	done
    done
}

function state_consistency_wait() {
    ret=`redis-cli -n 0 keys '*' | grep 'health' | grep 'health'`
    ret=`echo "$ret" | head -1`
    first_dev=${ret%:health}
    first_dev=${first_dev#daq_service:*:}

    ret=`redis-cli -n 0 keys '*' | grep 'health' `
    #echo "$ret"
    for str in $ret
    do
	dev=${str%:health}
	dev=${dev#daq_service:*:}
	#echo "$dev"
	for i in {1..10}
	do
	    first_dev_state=`redis-cli -n 1 hget metrics:state ${first_dev}`
	    state=`redis-cli -n 1 hget metrics:state ${dev}`
	    #echo $state
	    if [ "$state" = "" ]; then
		echo "Null state"
		sleep 1
		continue
	    fi
	    if [ "$state" = "$first_dev_state" ]; then
		break
	    else
		#echo ""
		#echo "Waiting..."
		#echo "Device: $dev"
		#echo "Required state: $req_state"
		#echo "Current state: $state..."
		sleep 1
	    fi
	done
	if [ "$i" = "10" ]; then
	    kill_fairmq_devices
	    break
	fi
    done
    consistent_state="$first_dev_state"
}

function state_wait() {
    req_state=$1
    state_consistency_wait
    if [ ! "$req_state" = "$consistent_state" ]; then
	echo "states are not consistent!!!"
    fi
}

function start_run() {
    redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "CONNECT",    "services": ["all"], "instances": ["all"] }' > /dev/null
    state_wait 'DEVICE READY'
    redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "INIT TASK",  "services": ["all"], "instances": ["all"] }'  > /dev/null
    state_wait 'READY'
    redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "RUN",        "services": ["all"], "instances": ["all"] }' > /dev/null
    state_wait 'RUNNING'
    run_num=`redis-cli -n 0 get run_info:run_number`
    echo ""
    echo ""
    echo "New run started..."
    echo "Current run number: $run_num"
    echo "Run start time: "`date +"%Y-%m-%d %H:%M:%S"`
}

function stop_run() {
    echo ""
    echo "Run stop time: "`date +"%Y-%m-%d %H:%M:%S"`
    redis-cli  'INCR' 'run_info:run_number' > /dev/null
    run_num=`redis-cli -n 0 get run_info:run_number`
    echo "Next run number: $run_num"
    state_consistency_wait
    if [ "$consistent_state" = "RUNNING" ]; then
	redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "STOP",         "services": ["all"], "instances": ["all"] }' > /dev/null
	state_wait 'READY'
	redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "RESET TASK",   "services": ["all"], "instances": ["all"] }' > /dev/null
	state_wait 'DEVICE READY'
	redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "RESET DEVICE", "services": ["all"], "instances": ["all"] }' > /dev/null
	state_wait 'IDLE'
    elif [ "$consistent_state" = "READY"  ]; then
	redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "RESET TASK",   "services": ["all"], "instances": ["all"] }' > /dev/null
	state_wait 'DEVICE READY'
	redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "RESET DEVICE", "services": ["all"], "instances": ["all"] }' > /dev/null
	state_wait 'IDLE'
    elif [  "$consistent_state" = "DEVICE READY"  ]; then
	redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "RESET DEVICE", "services": ["all"], "instances": ["all"] }' > /dev/null
	state_wait 'IDLE'
    fi
}

function status() {
    ret=`redis-cli -n 0 keys '*' | grep 'health' | grep 'health'`
    ret=`echo "$ret" | head -1`
    first_dev=${ret%:health}
    first_dev=${first_dev#daq_service:*:}

    ret=`redis-cli -n 0 keys '*' | grep 'health' `
    #echo "$ret"
    for str in $ret
    do
	dev=${str%:health}
	dev=${dev#daq_service:*:}
	#echo "$dev"

        state=`redis-cli -n 1 hget metrics:state ${dev}`
        if [ "$state" = "" ]; then
		state="NULL"
        fi
        echo ${state} ":" ${dev}

#	for i in {1..10}
#	do
#	    first_dev_state=`redis-cli -n 1 hget metrics:state ${first_dev}`
#	    state=`redis-cli -n 1 hget metrics:state ${dev}`
#	    echo ${state} ":" ${dev}
#	    if [ "$state" = "" ]; then
#		    echo "Null state"
#		    sleep 1
#		    continue
#	    fi
#	    if [ "$state" = "$first_dev_state" ]; then
#		    break
#	    else
#    		#echo ""
#	    	#echo "Waiting..."
#    		#echo "Device: $dev"
#	    	#echo "Required state: $req_state"
# 	    	#echo "Current state: $state..."
#		    sleep 1
#	    fi
#	done
	##if [ "$i" = "10" ]; then
	##    kill_fairmq_devices
	##    break
	##fi
    done
    consistent_state="$first_dev_state"
}

function state_change() {
    sc_com=$1
    if [$2 = ""] ; then
        sc_pro="all"
    else
        sc_pro=$2
    fi

    if [ $sc_com = "initdev" ] ; then
        redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "CONNECT",    "services": ["all"], "instances": ["all"] }' > /dev/null
    	echo $sc_com
    elif [ $sc_com = "inittask" ] ; then
        redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "INIT TASK",    "services": ["all"], "instances": ["all"] }' > /dev/null
    	echo $sc_com
    elif [ $sc_com = "run" ] ; then
        redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "RUN",    "services": ["all"], "instances": ["all"] }' > /dev/null
    elif [ $sc_com = "stop" ] ; then
        redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "STOP",    "services": ["all"], "instances": ["all"] }' > /dev/null
    elif [ $sc_com = "resettask" ] ; then
        redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "RESET TASK",    "services": ["all"], "instances": ["all"] }' > /dev/null
    elif [ $sc_com = "resetdev" ] ; then
        redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "RESET DEVICE",    "services": ["all"], "instances": ["all"] }' > /dev/null
    elif [ $sc_com = "end" ] ; then
        redis-cli  'PUBLISH' 'daqctl' '{"command": "change_state", "value": "END",    "services": [$sc_pro], "instances": [$sc_pro] }' > /dev/null
    fi
}


script_name=$0

if [ $# -eq 0 ]
then
	com="status"
else
	com=$1
fi

if [ $com = "initdev" ] ; then
	state_change "initdev"
elif [ $com = "inittask" ] ; then
	state_change "inittask"
elif [ $com = "run" ] ; then
	state_change "run"
elif [ $com = "stop" ] ; then
	state_change "stop"
elif [ $com = "resettask" ] ; then
	state_change "resettask"
elif [ $com = "resetdev" ] ; then
	state_change "resetdev"
elif [ $com = "end" ] ; then
	state_change "end" $2
elif [ $com = "rstart" ] ; then
	start_run
elif [ $com = "rstop" ] ; then
	stop_run
elif [ $com = "status" ] ; then
    status
    #echo $consistent_state ": Gloval state"
else
    echo "unrecognized command " $com
fi

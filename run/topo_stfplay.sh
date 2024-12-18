#!/bin/bash

#host=127.0.0.1
#port=6379
#db=0
server=redis://127.0.0.1:6379/0

#function config_endpoint () {
function endpoint () {
  # Usage: 
  #   config_endpoint "service" "channel" "parameters"
  
  echo redis-cli -u $server hset daq_service:topology:endpoint:$1:$2 ${@:3}  
  redis-cli -u $server hset daq_service:topology:endpoint:$1:$2 ${@:3}  
}

#function config_link () {
function link () {
  # config_link "service1" "channel" "service2" "channel" "parameters"
  
  echo redis-cli -u $server set daq_service:topology:link:$1:$2,$3:$4 non
  redis-cli -u $server set daq_service:topology:link:$1:$2,$3:$4 none
}


echo "Clear DAQ service keys"
redis-cli -u $server keys 'daq_service:*' | xargs redis-cli -u $server del

echo "---------------------------------------------------------------------"
echo " config endpoint (socket)"
echo "---------------------------------------------------------------------"
#---------------------------------------------------------------------------
#            service        channel         options
#---------------------------------------------------------------------------

# Sampler 
#endpoint     Sampler          data           type push  method bind 
#endpoint     BenchmarkSampler       out           type push  method bind 
#endpoint     HulStrTdcEmulator      out           type push  method bind 
#endpoint     tdcemulator      out            type pair  method bind 

#
#endpoint     STFBuilder        in            type pair  method connect 
#endpoint     STFBuilder        out           type push  method connect autoSubChannel true
endpoint     STFBFilePlayer    out           type push  method connect 

#
endpoint     TimeFrameBuilder  in            type pull  method bind
endpoint     TimeFrameBuilder  out           type push  method connect autoSubChannel true 
endpoint     TFBFilePlayer     out           type push  method connect 

#
endpoint     LogicFilter       in            type pull  method bind
endpoint     LogicFilter       out           type push  method connect autoSubChannel true
endpoint     LogicFilter       dqm           type pub   method bind
endpoint     TriggerView       in            type sub   method connect

#
endpoint     FileSink          in            type pull  method bind
endpoint     tfdump            in            type pull  method bind


echo "---------------------------------------------------------------------"
echo " config link"
echo "---------------------------------------------------------------------"
#---------------------------------------------------------------------------
#       service1          channel1       service2     channel2      
#---------------------------------------------------------------------------

#
#link    TFBFilePlayer     out            LogicFilter       in
#link    LogicFilter       out            tfdump            in
#link    LogicFilter       dqm            TriggerView       in

#
link    STFBFilePlayer    out            TimeFrameBuilder  in
link    TimeFrameBuilder  out            tfdump            in

#
#link    STFBFilePlayer    out            TimeFrameBuilder  in
#link    TimeFrameBuilder  out            LogicFilter       in
#link    LogicFilter       out            tfdump            in
#
#link    STFBFilePlayer    out            tfdump            in
#link    TFBFilePlayer     out            tfdump            in

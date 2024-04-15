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


echo "---------------------------------------------------------------------"
echo " config endpoint (socket)"
echo "---------------------------------------------------------------------"
#---------------------------------------------------------------------------
#         service           channel       options
#---------------------------------------------------------------------------

endpoint  STFBFilePlayer    out           type push  method connect 

endpoint  TimeFrameBuilder  in            type pull  method bind
endpoint  TimeFrameBuilder  out           type pull  method bind
endpoint  TFBFilePlayer     out           type push  method bind 

endpoint  fltcoin           in            type pull  method connect
endpoint  fltcoin           out           type push  method bind 

endpoint  flttrig           in            type pull  method connect
endpoint  flttrig           out           type push  method bind 

endpoint  LogicFilter       in            type pull  method connect
endpoint  LogicFilter       out           type push  method bind 

endpoint  tfdump            in            type pull  method connect
endpoint  Sink              in            type pull  method connect
endpoint  FileSink          in            type pull  method connect


echo "---------------------------------------------------------------------"
echo " config link"
echo "---------------------------------------------------------------------"
#---------------------------------------------------------------------------
#         service1          channel1       service2          channel2      
#---------------------------------------------------------------------------

link      STFBFilePlayer    out            TimeFrameBuilder  in
link      TimeFrameBuilder  out            LogicFilter       in

link      TFBFilePlayer     out            LogicFilter       in
link      LogicFilter       out            tfdump            in

#link      TFBFilePlayer     out            tfdump            in

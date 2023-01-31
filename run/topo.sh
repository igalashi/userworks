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
#            service        channel         options
#---------------------------------------------------------------------------

# Sampler 
endpoint     Sampler          data           type push  method bind 
endpoint     BenchmarkSampler       out           type push  method bind 
endpoint     HulStrTdcEmulator      out           type push  method bind 
endpoint     tdcemulator      out            type pair  method bind 

#
endpoint     stfbuilder       in            type pair  method connect 
endpoint     stfbuilder       out           type push  method bind autoSubChannel true
#endpoint     stfbuilder       out           type pair  method bind
#endpoint     stfbuilder       dqm           type push  method bind 

#
endpoint     tfbuilder       in            type pull  method connect autoSubChannel true
endpoint     tfbuilder       out           type push  method bind 

# Sink
#endpoint     sink          in              type pull  method connect
#endpoint     Sink          in              type pull  method connect
#endpoint     MQSink        in              type pull  method connect
endpoint     tfdump        in              type pull  method connect
#endpoint     tfdump        in              type pair  method connect


echo "---------------------------------------------------------------------"
echo " config link"
echo "---------------------------------------------------------------------"
#---------------------------------------------------------------------------
#       service1         channel1        service2     channel2      
#---------------------------------------------------------------------------

#link    Sampler           data           Sink         in
#link    BenchmarkSampler  out            MQSink       in
link    tdcemulator       out            stfbuilder   in
link    stfbuilder        out            tfbuilder    in
link    tfbuilder         out            tfdump       in
#link    stfbuilder        out            tfdump    in

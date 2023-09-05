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
#endpoint     Sampler          data           type push  method bind 
#endpoint     BenchmarkSampler       out           type push  method bind 
#endpoint     HulStrTdcEmulator      out           type push  method bind 
#endpoint     tdcemulator      out            type pair  method bind 
endpoint     RecbeSampler     out           type push method connect

#
#endpoint     stfbuilder       in            type pair  method connect 
#endpoint     stfbuilder       out           type push  method connect autoSubChannel true
#endpoint     stfbuilder       dqm           type push  method bind 
#endpoint     STFBFilePlayer   out           type push  method connect 

#
#endpoint     tfbuilder       in            type pull  method bind autoSubChannel true
endpoint     tfbuilder       in            type pull  method bind
endpoint     tfbuilder       out           type push  method bind 
endpoint     TimeFrameBuilder  in          type pull  method bind
endpoint     TimeFrameBuilder  out         type push  method bind 

endpoint     TFBFilePlayer   out           type push  method bind 

#
endpoint     fltcoin        in             type pull  method connect
endpoint     fltcoin        out            type push  method bind


# Sink
#endpoint     sink          in              type pull  method connect
#endpoint     Sink          in              type pull  method connect
#endpoint     MQSink        in              type pull  method connect
#endpoint     tfdump        in              type pull  method connect
endpoint     rawdump       in              type pull  method connect


echo "---------------------------------------------------------------------"
echo " config link"
echo "---------------------------------------------------------------------"
#---------------------------------------------------------------------------
#       service1          channel1       service2     channel2      
#---------------------------------------------------------------------------

#link    Sampler           data           Sink         in
#link    BenchmarkSampler  out            MQSink       in

#link    tdcemulator       out            stfbuilder   in
#link    stfbuilder        out            tfdump       in

#link    tdcemulator       out            stfbuilder   in
#link    stfbuilder        out            tfbuilder    in
#link    tfbuilder         out            tfdump       in

#link    TFBFilePlayer     out            tfdump       in
#link    STFBFilePlayer    out            tfdump       in

#link    TFBFilePlayer     out            fltcoin      in
#link    fltcoin           out            tfdump       in

#link    STFBFilePlayer    out            TimeFrameBuilder in
#link    TimeFrameBuilder  out            fltcoin          in
#link    fltcoin           out            tfdump           in

#link    STFBFilePlayer    out            tfbuilder        in
#link    tfbuilder         out            fltcoin          in
#link    fltcoin           out            tfdump           in

link    RecbeSampler       out            TimeFrameBuilder in
link    TimeFrameBuilder   out            rawdump          in

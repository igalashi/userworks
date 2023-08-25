#!/bin/bash

#MY_TERM=xterm

DAQSERVICE_URI=' --registry-uri tcp://127.0.0.1:6379/0'
METRICS_URI=' --metrics-uri tcp://127.0.0.1:6379/1'
CONFIG_URI=' --parameter-config-uri tcp://127.0.0.1:6379/2'

#---------------------------------------------------------
#if [[ $1 =~ fairmq- ]]; then
#  BINDIR=""
#else 
#  BINDIR=/home/nestdaq/nestdaq/bin
#fi
#BINDIR=/home/nestdaq/nestdaq/bin
BINDIR=/home/nestdaq/nestdaq/src/userworks/build/recbe

#export LD_LIBRARY_PATH=$ROOTSYS/lib

PLUGIN_LIBDIR=/home/nestdaq/nestdaq/lib64

PLUGIN_SEARCH_PATH=" -S '<$PLUGIN_LIBDIR'"
DAQSERVICE_PLUGIN=" -P daq_service"
METRICS_PLUGIN=" -P metrics"
CONFIG_PLUGIN=" -P parameter_config"

echo "BINDIR             = $BINDIR"
echo "PLUGIN_LIBDIR      = $PLUGIN_LIBDIR"
echo "PLUGIN_SEARCH_PATH = $PLUGIN_SEARCH_PATH"
echo "DAQSERVICE_PLUGIN  = $DAQSERVICE_PLUGIN"
echo "METRICS_PLUGIN     = $METRICS_PLUGIN"
echo "CONFIG_PLUGIN      = $CONFIG_PLUGIN"
echo "DAQSERVICE_URI     = $DAQSERVICE_URI"
echo "METRICS_URI        = $METRICS_URI"
echo "CONFIG_URI         = $CONFIG_URI"

#==============================================================================
function run_device () {
  local var;
  var+=$PLUGIN_SEARCH_PATH
  var+=$DAQSERVICE_PLUGIN
  var+=$METRICS_PLUGIN
  var+=$CONFIG_PLUGIN
  var+=$DAQSERVICE_URI
  var+=$METRICS_URI
  var+=$CONFIG_URI
  var+=" --severity debug"

  if [ -n "$MY_TERM" ]; then
    #echo \
    $MY_TERM -title $1 -e "$BINDIR/$@ $var"
  else
    #echo \
    eval "$BINDIR/$@ $var"
  fi
}

run_device $@


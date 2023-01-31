#!/bin/sh

xterm -T TDCEmu -e ./start_device.sh tdcemulator &
xterm -T TDCEmu -e ./start_device.sh tdcemulator &
xterm -T STFBuilder -e ./start_device.sh stfbuilder --fem-id 00.00.00.01 &
xterm -T STFBuilder -e ./start_device.sh stfbuilder --fem-id 00.00.00.02 &
#xterm -T TFBuilder -e ./start_device.sh tfbuilder &
#xterm -T tfdump -e ./start_device.sh tfdump &

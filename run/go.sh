#!/bin/sh

#xterm -T TDCEmu -e ./start_device.sh tdcemulator &
#xterm -T TDCEmu -e ./start_device.sh tdcemulator &
#xterm -T STFBuilder -e ./start_device.sh stfbuilder --fem-id 00.00.00.01 &
#xterm -T STFBuilder -e ./start_device.sh stfbuilder --fem-id 00.00.00.02 &
#xterm -T TFBuilder -e ./start_device.sh tfbuilder &
#xterm -T tfdump -e ./start_device.sh tfdump &

xterm -geometry 100x20 -T TDCEmu -e ./start_device.sh ../src/nestdaq-user-impl/bin/Sampler &
#xterm -T TDCEmu -e ./start_device.sh ../src/nestdaq-user-impl/bin/Sampler &
xterm -geometry 100x20 -T STFBuilder -e ./start_device.sh ../src/nestdaq-user-impl/bin/STFBuilder &
#xterm -T STFBuilder -e ./start_device.sh ../src/nestdaq-user-impl/bin/STFBuilder &
xterm -geometry 100x20 -T TFBuilder -e ./start_device.sh ../src/nestdaq-user-impl/bin/TimeFrameBuilder &
sleep 1
xterm -geometry 100x50 -T FLTCoin -e ./start_device.sh fltcoin &
xterm -geometry 100x40 -T tfdump -e ./start_device.sh tfdump &

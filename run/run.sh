#!/bin/bash

#for runID in {0..20}
#do
#    echo "start device Sampler ${runID}"
#    xterm -geometry 100x20+0+`expr ${runID} \* 240` -e ./uudevice.sh RecbeSampler --ip 127.0.0.1 --port `expr 8024 + ${runID}` &
#    sleep 0.2
#done


tmux new-session -d -s sampler
for runID in {0..99}
do
    echo "start device Sampler ${runID}"
    tmux new-window -d -n S${runID} -t sampler -- "./uudevice.sh RecbeSampler --ip 127.0.0.1 --port `expr 9000 + ${runID}`" &
    sleep 0.1
done
tmux kill-window -t sampler:0
xterm -geometry 80x20+0+0 -T Sampler -e tmux a -t sampler &


#for runID in {0..0}
#do
#    echo "start device SubTime Frame Builder ${runID}"
#    ./start_device.sh.in STFBuilder
#    sleep 0.2
#done

for runID in {0..0}
do
    echo "start device TimeFrameBuilder"
    xterm -geometry 100x20+400+0 -e ./uudevice.sh TimeFrameBuilder &
    sleep 0.2
done


xterm -geometry 100x40+400+400 -e ./uudevice.sh rawdump &

#for runID in {0..19}
#do
#    echo "start device FileSink"
#    ./start_device.sh.in FileSink
#    sleep 0.2
#done

#for runID in {0}
#do
#    echo "start device AmQStrTdcDqm"
#    ./start_device.sh.in AmQStrTdcDqm
#    sleep 0.2
#done

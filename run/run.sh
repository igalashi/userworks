#!/bin/bash

#for runID in {0..20}
#do
#    echo "start device Sampler ${runID}"
#    xterm -geometry 100x20+0+`expr ${runID} \* 240` -e ./uudevice.sh RecbeSampler --ip 127.0.0.1 --port `expr 8024 + ${runID}` &
#    sleep 0.2
#done


tmux new-session -d -s sampler
for runID in {0..90}
do
    echo "start device Sampler ${runID}"
    #tmux new-window -d -n S${runID} -t sampler -- "./uudevice.sh RecbeSampler --ip 127.0.0.1 --port `expr 9000 + ${runID}`" &
    tmux new-window -d -n S${runID} -t sampler -- "./uudevice.sh RecbeSampler --mode 1" &
    sleep 0.1
done
tmux kill-window -t sampler:0
xterm -geometry 120x20+0+0 -T Sampler -e tmux a -t sampler &


#for runID in {0..0}
#do
#    echo "start device SubTime Frame Builder ${runID}"
#    ./start_device.sh.in STFBuilder
#    sleep 0.2
#done

tmux new-session -d -s TFB
for runID in {0..0}
do
    echo "start device RecbeTimeFrameBuilder"
    tmux new-window -d -n S${runID} -t TFB -- "./uudevice.sh RecbeTimeFrameBuilder" &
    #xterm -geometry 100x20+0+330 -e ./uudevice.sh RecbeTimeFrameBuilder &
    sleep 0.1
done
tmux kill-window -t TFB:0
xterm -geometry 100x20+0+330 -T TFB -e tmux a -t TFB &


xterm -geometry 100x20+620+330 -e ./uudevice.sh RecbeDisplay &
#xterm -geometry 100x20+620+440 -e ./uudevice.sh RecbeDisplay &
#xterm -geometry 100x20+600+480 -e ./uudevice.sh RecbeDisplay &

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

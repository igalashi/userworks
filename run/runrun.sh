#!/bin/bash

if [ $# -gt 0 ];
then
    TFB=$1
else
    TFB='tfb'
fi

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/local/lib:$HOME/local/lib64

tmux new-session -d -t sampler
for runID in {0..19}
do
    echo "start device Sampler ${runID}"
    #./start_device.sh.in AmQStrTdcSampler
    tmux new-window -d -n S${runID} -t sampler -- ./start_device.sh AmQStrTdcSampler
    sleep 0.1
done
tmux kill-window -t sampler:0
xterm -geometry 80x15+0+0 -T Sampler -e tmux a -t sampler &


tmux new-session -d -t stfb
for runID in {0..19}
do
    echo "start device SubTime Frame Builder ${runID}"
	tmux new-window -d -n STF${runID} -t stfb -- ./start_device.sh STFBuilder
    sleep 0.1
done
tmux kill-window -t stfb:0
xterm -geometry 80x15+500+0 -T SubTimeFrameBuilder -e tmux a -t stfb &

#if [ 0 == 1 ]
#then

if [ $TFB = "tfb" ];
then
	tmux new-session -d -t tfb
	for runID in {0..3}
	#for runID in {0}
	do
	    echo "start device TimeFrameBuilder ${runID}"
	#    ./start_device.sh.in TimeFrameBuilder
	     tmux new-window -d -n STF${runID} -t tfb -- ./start_device.sh TimeFrameBuilder
	     #tmux new-window -d -n STF${runID} -t tfb -- ./ustart.sh TimeFrameBuilder
	    sleep 0.2
	done
	tmux kill-window -t tfb:0
	xterm -geometry 100x24 -T TimeFrameBuilder -e tmux a -t tfb &

	tmux new-session -d -t sink
	for runID in {0..0}
	do
	    echo "start device FileSink ${runID}"
		tmux new-window -d -n STF${runID} -t sink -- ./start_device.sh FileSink
		#tmux new-window -d -n STF${runID} -t sink -- ./start_device.sh tfdump
	    sleep 0.2
	done
	tmux kill-window -t sink:0
	xterm -geometry 80x15 -T Sink -e tmux a -t sink &

else

	tmux new-session -d -t sink
	for runID in {0..19}
	do
	    echo "start device FileSink ${runID}"
		tmux new-window -d -n STF${runID} -t sink -- ./start_device.sh FileSink
	    sleep 0.2
	done
	tmux kill-window -t sink:0
	xterm -geometry 80x15+500+270 -T Sink -e tmux a -t sink &

fi

#fi

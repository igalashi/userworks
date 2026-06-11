#!/bin/bash

#RUN=1184
#RUN=1188
RUN=1194

if [ $# -eq 0 ]
then
	RUN=1194
else
	RUN=$1
fi

tmux new-session -d -s STFPlay

#echo "start device TFPlay"
#tmux new-window -d -n S${runID} -t TFPlay -- "./udevice.sh TFBFilePlayer --wait 10 --in-file $HOME/nestdaq/data/run001132.dat " &

#for i in {00..09}
#for i in 01 02 03 04 05 06 07 08 09 10 11
for i in 01 02 03 04 05 06 07 08 09 10 11
do
	tmux new-window -d -n S${i} -t STFPlay -- \
		"./udevice.sh STFBFilePlayer --wait 50 --in-file $HOME/nestdaq/data/${i}/run00${RUN}.dat " &
	sleep 0.1
done

#tmux new-window -d -n TFB -t STFPlay -- "./udevice.sh TimeFrameBuilder" &
##tmux new-window -d -n S${runID} -t STFPlay -- "./udevice.sh LogicFilter" &
#tmux new-window -d -n DUMP -t STFPlay -- "./uudevice.sh tfdump --shrink true" &

tmux kill-window -t STFPlay:0

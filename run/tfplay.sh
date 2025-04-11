#!/bin/bash

tmux new-session -d -s TFPlay

echo "start device TFPlay"
tmux new-window -d -n S${runID} -t TFPlay -- "./udevice.sh TFBFilePlayer --wait 50 --in-file $1" &
#tmux new-window -d -n S${runID} -t TFPlay -- "./udevice.sh TFBFilePlayer --wait 10 --in-file $HOME/nestdaq/run001132.dat " &
tmux new-window -d -n S${runID} -t TFPlay -- "./uudevice.sh LogicFilter" &
#tmux new-window -d -n S${runID} -t TFPlay -- "./udevice.sh LogicFilter" &
tmux new-window -d -n S${runID} -t TFPlay -- "./uudevice.sh tfdump --shrink true" &
#tmux new-window -d -n S${runID} -t TFPlay -- "./uudevice.sh RecbeDisplaye" &
#tmux new-window -d -n S${runID} -t TFPlay -- "./uudevice.sh hdtbldisplay" &
#tmux new-window -d -n S${runID} -t TFPlay -- "./udevice.sh TriggerView" &
tmux new-window -d -n S${runID} -t TFPlay -- "./uudevice.sh TriggerView" &
sleep 0.1

tmux kill-window -t TFPlay:0

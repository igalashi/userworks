#!/bin/bash

tmux new-session -d -s TFPlay

echo "start device TFPlay"
tmux new-window -d -n S${runID} -t TFPlay -- "./udevice.sh TFBFilePlayer --wait 10 --in-file $1" &
#tmux new-window -d -n S${runID} -t TFPlay -- "./udevice.sh TFBFilePlayer --wait 10 --in-file $HOME/nestdaq/run001132.dat " &
#tmux new-window -d -n S${runID} -t TFPlay -- "./udevice.sh LogicFilter" &
#tmux new-window -d -n S${runID} -t TFPlay -- "./uudevice.sh tfdump --shrink true" &
#tmux new-window -d -n S${runID} -t TFPlay -- "./uudevice.sh RecbeDisplaye" &
tmux new-window -d -n S${runID} -t TFPlay -- "./uudevice.sh hdtbldisplay" &
sleep 0.1

tmux kill-window -t TFPlay:0

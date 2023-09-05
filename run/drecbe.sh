#!/bin/sh

EXECPATH=$NESTDAQ/src/userworks/build/recbe
tmux new-session -d -s drecbe
for runID in {0..99}
do
    echo "start drecbe ${runID}"
    tmux new-window -d -n ${runID} -t drecbe -- "$EXECPATH/drecbe --id ${runID} --host 127.0.0.1 --port `expr 9000 + ${runID}` --freq 100"
    sleep 0.1
done
tmux kill-window -t drecbe:0

#xterm -geometry 80x15+0+0 -T Sampler -e tmux a -t drecbe &
tmux a -t drecbe

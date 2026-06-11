#!/bin/bash

echo '\$1:', $1, ' \$2', $2, ' \$3', $3
CH=$2
VAL=`printf "%04x" "$3"`
echo fct $CH 0xbe12$VAL

cd $HOME/runfc7/fc7/
source ./setup.sh

cd $HOME/runfc7/fc7/tests/myscript
./runnoinit << _ENDFCT_
fct(10, 0xbe120000)
fct(11, 0xbe120000)
fct(12, 0xbe120000)
fct(13, 0xbe120000)
fct(14, 0xbe120000)
fct(15, 0xbe120000)
fct(16, 0xbe120000)
fct($CH, 0xbe12$VAL)
_ENDFCT_

cd $HOME/runfc7
./jtagrecbe.sh << _ENDJTAG_
s
_ENDJTAG_

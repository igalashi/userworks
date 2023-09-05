#!/bin/bash

server=redis://127.0.0.1:6379/2

function param () {
  # "instance":"field" "value"
  #echo redis-cli -u $server set parameters:$1:$2 ${@:3}
  #redis-cli -u $server set parameters:$1:$2 ${@:3}
  echo redis-cli -u $server hset parameters:$1 ${@:2}
  redis-cli -u $server hset parameters:$1 ${@:2}
}

#===============================================================================================
# RLTDC 161~167, 170  HRTDC 168, 169
#===============================================================================================
#      isntance-id                field       value             field       value  
#HR-TDC
param  AmQStrTdcSampler-0         msiTcpIp   192.168.2.169   TdcType     2
param  AmQStrTdcSampler-1         msiTcpIp   192.168.2.170   TdcType     2
#LR-TDC A
param  AmQStrTdcSampler-2         msiTcpIp   192.168.2.161   TdcType     3
param  AmQStrTdcSampler-3         msiTcpIp   192.168.2.162   TdcType     3
param  AmQStrTdcSampler-4         msiTcpIp   192.168.2.163   TdcType     3
param  AmQStrTdcSampler-5         msiTcpIp   192.168.2.164   TdcType     3
param  AmQStrTdcSampler-6         msiTcpIp   192.168.2.165   TdcType     3
param  AmQStrTdcSampler-7         msiTcpIp   192.168.2.166   TdcType     3
param  AmQStrTdcSampler-8         msiTcpIp   192.168.2.167   TdcType     3
param  AmQStrTdcSampler-9         msiTcpIp   192.168.2.168   TdcType     3
#LR-TDC B
param  AmQStrTdcSampler-10        msiTcpIp   192.168.2.173   TdcType     3
param  AmQStrTdcSampler-11        msiTcpIp   192.168.2.174   TdcType     3
param  AmQStrTdcSampler-12        msiTcpIp   192.168.2.175   TdcType     3
param  AmQStrTdcSampler-13        msiTcpIp   192.168.2.176   TdcType     3
param  AmQStrTdcSampler-14        msiTcpIp   192.168.2.177   TdcType     3
param  AmQStrTdcSampler-15        msiTcpIp   192.168.2.178   TdcType     3
param  AmQStrTdcSampler-16        msiTcpIp   192.168.2.179   TdcType     3
# MIKUMARI-family
param  AmQStrTdcSampler-17        msiTcpIp   192.168.2.160   TdcType     3
param  AmQStrTdcSampler-18        msiTcpIp   192.168.2.171   TdcType     3
param  AmQStrTdcSampler-19        msiTcpIp   192.168.2.172   TdcType     3
#
param  STFBuilder-0  max-hbf 1
param  STFBuilder-1  max-hbf 1
param  STFBuilder-2  max-hbf 1
param  STFBuilder-3  max-hbf 1
param  STFBuilder-4  max-hbf 1
param  STFBuilder-5  max-hbf 1
param  STFBuilder-6  max-hbf 1
param  STFBuilder-7  max-hbf 1
param  STFBuilder-8  max-hbf 1
param  STFBuilder-9  max-hbf 1
param  STFBuilder-10 max-hbf 1
param  STFBuilder-11 max-hbf 1
param  STFBuilder-12 max-hbf 1
param  STFBuilder-13 max-hbf 1
param  STFBuilder-14 max-hbf 1
param  STFBuilder-15 max-hbf 1
param  STFBuilder-16 max-hbf 1
param  STFBuilder-17 max-hbf 1
param  STFBuilder-18 max-hbf 1
param  STFBuilder-19 max-hbf 1
#
#
param FileSink-0  multipart true openmode create prefix data/00 ext .dat 
param FileSink-1  multipart true openmode create prefix data/01 ext .dat 
param FileSink-2  multipart true openmode create prefix data/02 ext .dat 
param FileSink-3  multipart true openmode create prefix data/03 ext .dat 
param FileSink-4  multipart true openmode create prefix data/04 ext .dat 
param FileSink-5  multipart true openmode create prefix data/05 ext .dat 
param FileSink-6  multipart true openmode create prefix data/06 ext .dat 
param FileSink-7  multipart true openmode create prefix data/07 ext .dat 
param FileSink-8  multipart true openmode create prefix data/08 ext .dat 
param FileSink-9  multipart true openmode create prefix data/09 ext .dat 
param FileSink-10 multipart true openmode create prefix data/10 ext .dat 
param FileSink-11 multipart true openmode create prefix data/11 ext .dat 
param FileSink-12 multipart true openmode create prefix data/12 ext .dat 
param FileSink-13 multipart true openmode create prefix data/13 ext .dat 
param FileSink-14 multipart true openmode create prefix data/14 ext .dat 
param FileSink-15 multipart true openmode create prefix data/15 ext .dat 
param FileSink-16 multipart true openmode create prefix data/16 ext .dat
param FileSink-17 multipart true openmode create prefix data/17 ext .dat
param FileSink-18 multipart true openmode create prefix data/18 ext .dat
param FileSink-19 multipart true openmode create prefix data/19 ext .dat

#param FileSink-0  multipart true openmode append path /dev/null 
#param FileSink-1  multipart true openmode append path /dev/null 
#param FileSink-2  multipart true openmode append path /dev/null 
#param FileSink-3  multipart true openmode append path /dev/null 
#param FileSink-4  multipart true openmode append path /dev/null 
#param FileSink-5  multipart true openmode append path /dev/null 
#param FileSink-6  multipart true openmode append path /dev/null 
#param FileSink-7  multipart true openmode append path /dev/null 
#param FileSink-8  multipart true openmode append path /dev/null 
#param FileSink-9  multipart true openmode append path /dev/null 
#param FileSink-10 multipart true openmode append path /dev/null 
#param FileSink-11 multipart true openmode append path /dev/null 
#param FileSink-12 multipart true openmode append path /dev/null 
#param FileSink-13 multipart true openmode append path /dev/null 
#param FileSink-14 multipart true openmode append path /dev/null 
#param FileSink-15 multipart true openmode append path /dev/null 
#param FileSink-16 multipart true openmode append path /dev/null

#param AmQStrTdcDqm-0 num-source 17

# Probably the following aren't used.
param TimeFrameBuilder-0 num-source 20
param TimeFrameBuilder-1 num-source 20
param TimeFrameBuilder-2 num-source 20
param TimeFrameBuilder-3 num-source 20
param TimeFrameBuilder-4 num-source 20
param TimeFrameBuilder-5 num-source 20

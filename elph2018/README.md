# DAQ classes

## DAQ classes derived from FairMQDevice  
Each user application class derived from FairMQDevice consists of following 3 files. 

* XXX.h  
* XXX.cxx  
* runXXX.cxx  

The function `main()` is defined in the header file of [runFairMQDevice.h](https://github.com/FairRootGroup/FairMQ/fairmq/runFairMQDevice.h) and it is included in runXXX.cxx.
You have to describe custom command-line options in `addCustomOptions()` with `boost::program_options::options_description` and a factory function `getDevice()` to create the user applicatoin device class in runXXX.cxx. 


## Examples for demonstration  
Some device classes are prepared for a demonstration of data transfer using FairMQ. 
These devices are implemented with tentative specification and data format. 
All messages are treated as a multi-part message (`FairMQParts`). 

#### BenchmarkSampler 
`BenchmarkSampler` generates a dummy payload. 
The implementation is base on `FairMQBenchmarkSampler` provided as a [generic FairMQDevice](https://github.com/FairRootGroup/FairMQ/fairmq/devices/). 
Output data is a `FairMQParts`, which contains a header part (`RAWHeader`) and a body part. 
`HearbeatFrameHeader` is embedded into the body part at specified position and rate. 
* input channel  
  * N/A
* output channel 
  * type = push or pub
  * method = bind or connect
     
#### SubTimeFrameBuilder  
`SubTimeFrameBuilder` receives data from `BenchmarkSampler` and finds `HeartbeatFrameHeader` in the received data. 
When a certain number of `HeartbeatFrameHeader` are found, `SubTimeFrameBuilder` creates a `FairMQParts` for output. 
The output data consists of a header part (`SubTimeFrameHeader`) and a body part. 
The body part may contains one or more `FairMQMessagePtr`s.  
The current implemenation assumes that the number of input channel must be one. 
* input channel
  * type = pull or sub
  * method = bind or connect
* output channel 
  * type = pull or sub
  * method = bind or connect

#### SubTimeFrameSender  
`SubTimeFrameSender` receives data from `SubTimeFrameBuilder` and distrbute them to `TimeFrameBuilder`s. 
The implementation is based on `FLPSender` of ALICE O2.
The data destination is selected by using a round-robin algorithm of `direction = time_frame_id % number_of_destination`. 
The current implemenation assumes: 
* The number of input and output channel must be one. 
* The output channel can manage different destinations (host:port). 
* The type and method of output channel must be **push** and **connect**, respectively. 
* The number of destination endpoints must be given by `--num-destination` option. 

* input channel 
  * type = pull or sub
  * method = bind or connect
* output channel 
  * **type = push**
  * **method = connect**

#### TimeFrameBuilder  
`TimeFrameBuilder` receives data from `SubTimeFrameSender`s by using a fair-queing algorithm. 
The implementation is based on `EPNReceiver` of ALICE O2. 
The current implementation assumes:
* The type and method of input channel must be **pull** and **bind**, respectively. 
* The number of source endpoints must be given by `--num-source` option.

* input channel 
  * **type = pull**
  * **method = bind**
* output channel 
  * type = push or pub
  * method = bind or connect

#### TimeStampInserter
`TimerStampInserter` inserts a FairMQMessagePtr of `TimeStampStampFrameHeader` in the received data. 

#### MQSink  
`MQSink` is a dummy device to receive any data of `FairMQParts` and does nothing. 

#### FileSink  
`FileSink` receives and write data to a file. 
You can select a data compression type, `.dat`, `.gz`, `.bz2`, `.xz` or `.root` (ROOT file) by `--file-type` option. 
When  ROOT file option is chosen, `TTree` with a branch of `std::vector<std::vector<uint8_t>>` is created. 

More examples are found at [generic devieces of FairMQDevice](https://github.com/FairRootGroup/FairMQ/fairmq/devices) and [ALCIE O2](https://github.com/AliceO2Group/AliceO2). 
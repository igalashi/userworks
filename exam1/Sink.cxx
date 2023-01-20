#include <chrono>
#include <thread>
#include <iomanip>

#include <runFairMQDevice.h>

#include "Sink.h"
#include "mstopwatch.cxx"
mStopWatch *g_sw;
#include "event.h"

static constexpr std::string_view MyClass{"Sink"};

namespace bpo = boost::program_options;

//_____________________________________________________________________________
void addCustomOptions(bpo::options_description &options)
{
  using opt = Sink::OptionKey;
  options.add_options()
  (opt::InputChannelName.data(),
   //bpo::value<std::string>()->default_value(opt::InputChannelName.data()),
   bpo::value<std::string>()->default_value("in"),
   "Name of input channel\n")
  //
  (opt::Multipart.data(),
   //bpo::value<std::string>()->default_value("true"),
   bpo::value<std::string>()->default_value("false"),
   "Handle multipart message\n");
}

//_____________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions &)
{
 return new Sink; 
}

//_____________________________________________________________________________
void PrintConfig(const fair::mq::ProgOptions* config, std::string_view name, std::string_view funcname)
{
  auto c = config->GetPropertiesAsStringStartingWith(name.data());
  std::ostringstream ss;
  ss << funcname << "\n\t " << name << "\n";
  for (const auto &[k, v] : c) {
    ss << "\t key = " << k << ", value = " << v << "\n";
  }
  LOG(debug) << ss.str();
} 

//_____________________________________________________________________________

bool Sink::HandleData(FairMQMessagePtr &msg, int index)
{

	char *inbuf = static_cast<char *>(msg->GetData());
	int inbuf_size = msg->GetSize();
	struct event_header *ieheader
		= reinterpret_cast<struct event_header *>(inbuf);
	struct node_header *inheader
		= reinterpret_cast<struct node_header *>(
		inbuf + sizeof(struct event_header));
	//char *icontainer = inbuf
	//	+ sizeof(struct event_header) + ieheader->node_header_size;


	#if 0
	unsigned int *data = reinterpret_cast<unsigned int*>(inbuf);
	for (int i = 0 ; i < 32 ; i++) {
		if ((i % 8) == 0) std::cout << std::endl << "## ";
		std::cout << " " << std::setfill('0') << std::setw(8) << std::hex << data[i];
	}
	std::cout << std::dec << std::endl;
	#endif

	#if 0
  	LOG(debug) << __FUNCTION__
		<< " received = " << ieheader->container_size
		<< " [" << index << "] " << fNumMessages
		<< " : "
		<< " " << static_cast<unsigned int>(icontainer[0])
		<< " " << static_cast<unsigned int>(icontainer[1])
		<< " " << static_cast<unsigned int>(icontainer[2])
		<< " " << static_cast<unsigned int>(icontainer[3]);
	#endif


	int npass = ieheader->node_header_size / sizeof(struct node_header);
	for (int i = 0 ; i < npass ; i++) {
		//LOG(debug) << "# " << inheader->type << " " << inheader->id_number;
		int type = inheader->type;
		int worker_id = -1;
		if (type == 0x57524b52) {
			worker_id = inheader->id_number;
			if (static_cast<int>(fnWorker.size()) <= worker_id) {
				fnWorker.resize(worker_id + 1);
				fnWorker[worker_id] = 0;
			}
			fnWorker[worker_id]++;
			//std::cout << "## " << worker_id
			//	<< " / " << fnWorker[worker_id] << std::endl;
		}

		inheader++;
	}
	
 
	static int counter = 0;
	counter++;
	int elapse = g_sw->elapse();
	if (elapse > 10 * 1000) {
  		LOG(debug) << __FUNCTION__
			<< " received = " << inbuf_size
			<< " data : "  << ieheader->container_size
			<< " [" << index << "] " << fNumMessages;
		LOG(info) << "Freq. "
			<< static_cast<double>(counter) / elapse
			<< " kHz";
		counter = 0;
		g_sw->restart();

		std::cout << "# Passed Worker " << fnWorker.size() << " : ";
		for (unsigned int i = 0 ; i < fnWorker.size() ; i++) {
			if ((i % 10) == 0) std::cout << std::endl;
			std::cout << " " << std::setfill('0') << std::setw(4) << i
				<< ":" << std::setfill(' ') << std::setw(4) << fnWorker[i];
		}
		std::cout << std::endl;

	}

	++fNumMessages;
	return true;
}

#if 0
bool Sink::HandleData(FairMQMessagePtr &msg, int index)
{
  const auto ptr = reinterpret_cast<char*>(msg->GetData());
  std::string s(ptr, ptr+msg->GetSize());
  //LOG(debug) << __FUNCTION__ << " received = " << s << " [" << index << "] " << fNumMessages;
 
	static int counter = 0;
	counter++;
	int elapse = g_sw->elapse();
	if (elapse > 10 * 1000) {
  		LOG(debug) << __FUNCTION__
			<< " received = " << s
			<< " [" << index << "] " << fNumMessages;
		LOG(info) << "Freq. "
			<< static_cast<double>(counter) / elapse
			<< " kHz";
		counter = 0;
		g_sw->restart();
	}

  ++fNumMessages;
  return true;
}
#endif

//_____________________________________________________________________________
bool Sink::HandleMultipartData(FairMQParts &msgParts, int index)
{
  for (const auto& msg : msgParts) {
    const auto ptr = reinterpret_cast<char*>(msg->GetData());
    std::string s(ptr, ptr+msg->GetSize());
    LOG(debug) << __FUNCTION__ << " received = " << s << " [" << index << "] " << fNumMessages;
    LOG(debug) << s;
    ++fNumMessages;
  }
 return true; 
}

//_____________________________________________________________________________
void Sink::Init()
{
  PrintConfig(fConfig, "channel-config", __PRETTY_FUNCTION__);
  PrintConfig(fConfig, "chans.", __PRETTY_FUNCTION__);

  fNumMessages = 0;

	fnWorker.clear();
	fnWorker.resize(0);

	g_sw = new mStopWatch();
	g_sw->start();
}

//_____________________________________________________________________________
void Sink::InitTask()
{
  PrintConfig(fConfig, "channel-config", __PRETTY_FUNCTION__);
  PrintConfig(fConfig, "chans.", __PRETTY_FUNCTION__);

  LOG(debug) << MyClass << " InitTask";
  using opt = OptionKey;  

  fInputChannelName = fConfig->GetProperty<std::string>(opt::InputChannelName.data());
  LOG(debug) << " input channel = " << fInputChannelName;

  const auto &isMultipart = fConfig->GetProperty<std::string>(opt::Multipart.data());
  if (isMultipart=="true" || isMultipart=="1") {
    LOG(warn) << " set multipart data handler";
    OnData(fInputChannelName, &Sink::HandleMultipartData);
  } else {
    LOG(warn) << " set data handler"; 
    OnData(fInputChannelName, &Sink::HandleData); 
  }

}

//_____________________________________________________________________________
void Sink::PostRun()
{
  using opt = OptionKey;  
  LOG(debug) << __func__;
  int nrecv=0;
  while (true) {
    const auto &isMultipart = fConfig->GetProperty<std::string>(opt::Multipart.data());
    if (isMultipart=="true" || isMultipart=="1") {
      FairMQParts parts;
      if (Receive(parts, fInputChannelName) <= 0) {
        LOG(debug) << __func__ << " no data received " << nrecv;
        ++nrecv;
        //if (nrecv>10) {
        if (nrecv > 3) {
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      } else {
        LOG(debug) << __func__ << " print data";
        HandleMultipartData(parts, 0);
      }
    } else {
      FairMQMessagePtr msg(NewMessage());
      if (Receive(msg, fInputChannelName) <= 0) {
        LOG(debug) << __func__ << " no data received " << nrecv;
        ++nrecv;
        //if (nrecv>10) {
        if (nrecv > 3) {
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      } else {
        LOG(debug) << __func__ << " print data";
        HandleData(msg, 0);
      }
    }
    LOG(debug) << __func__ << " done";
  }
}

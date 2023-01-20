#include <sstream>
#include <string>
#include <atomic>

#include <runFairMQDevice.h>
#include "dummySampler.h"

#include "mstopwatch.cxx"
mStopWatch *g_sw;

namespace bpo = boost::program_options;

//_____________________________________________________________________________
void addCustomOptions(bpo::options_description& options)
{
  options.add_options()
  ("text",
    bpo::value<std::string>()->default_value("Hello"),
    "Text to send out")
  ("max-iterations",
    bpo::value<std::string>()->default_value("0"),
    "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
} 

//_____________________________________________________________________________
FairMQDevicePtr getDevice(const fair::mq::ProgOptions& /*config*/)
{
  return new Sampler(); 
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
Sampler::Sampler()
  : fText()
  , fMaxIterations(0)
  , fNumIterations(0)
  , fId(0)
{
  LOG(debug) << "Sampler : hello";
}

//_____________________________________________________________________________
//Sampler::~Sampler()
//{
// unsubscribe to property change  
//  fConfig->UnsubscribeAsString("Sampler");
//  LOG(debug) << "Sampler : bye";
//}

//_____________________________________________________________________________
#include <assert.h>
void Sampler::Init()
{
// subscribe to property change  
//  fConfig->SubscribeAsString("Sampler", [](const std::string& key, std::string value){
//    LOG(debug) << "Sampler (subscribe) : key = " << key << ", value = " << value;
//  });
  PrintConfig(fConfig, "channel-config", __PRETTY_FUNCTION__);
  PrintConfig(fConfig, "chans.", __PRETTY_FUNCTION__);

	std::string name = fConfig->GetProperty<std::string>("id");
	std::string snum = name.substr(name.rfind("-") + 1);
	std::istringstream ss(snum);
	ss >> fId;

	//std::cout << "##### fId : " << fId << std::endl;

	g_sw = new mStopWatch();
	g_sw->start();
}


//_____________________________________________________________________________
void Sampler::InitTask()
{
  PrintConfig(fConfig, "channel-config", __PRETTY_FUNCTION__);
  PrintConfig(fConfig, "chans.", __PRETTY_FUNCTION__);

  fText = fConfig->GetProperty<std::string>("text");
  fMaxIterations = std::stoull(fConfig->GetProperty<std::string>("max-iterations"));
}

//_____________________________________________________________________________

#include "dummy_event.h"
//const int container_size = 1024;
const int container_size = 1024 * 50;

int wait_trigger()
{
	//usleep(1*1000);
	//usleep(1);
	return 0;
}

int generate_dummy(char *data, int csize, int id)
{
	struct event_header *eheader = reinterpret_cast<struct event_header *>(
		data);
	struct node_header *nheader = reinterpret_cast<struct node_header *>(
		data + sizeof(struct event_header));
	char *container = data
		+ sizeof(struct event_header)
		+ sizeof(struct node_header);

	eheader->magic = EVENT_MAGIC;
	eheader->node_header_size = sizeof(struct node_header) * 1;
	eheader->container_size = csize;
	nheader->type = 0x534d5052;
	nheader->id_number = static_cast<uint32_t>(id);

	//for (int i = 0 ; i < csize ; i++) {
	for (int i = 0 ; i < 1024 ; i++) {
		container[i] = static_cast<char>(i & 0xff);
	}

	int data_size = 
		+ sizeof(struct event_header)
		+ sizeof(struct node_header) * 1
		+ csize;

	return data_size;
}

std::atomic<int> gQdepth;

bool Sampler::ConditionalRun()
{

	wait_trigger();

	char *buf = new char[container_size
		+ sizeof(event_header) + sizeof(node_header)];
	gQdepth++;
	int data_size = generate_dummy(buf, container_size, fId);

	FairMQMessagePtr msg(
		NewMessage(
			buf,
			data_size,
			//[](void *data, void* object) { 
			[](void *data, void* ) { 
				char *p = reinterpret_cast<char *>(data);
				delete p;
				gQdepth--;
			}, 
			nullptr
		)
	);

	if (Send(msg, "data") < 0) {
		LOG(warn) << " event: " << fNumIterations
			<< " Send err.";
		return false;
	} else { 
		++fNumIterations;
		if (fMaxIterations > 0 && fNumIterations >= fMaxIterations) {
			LOG(info)
			<< "Configured maximum number of iterations reached."
			<< " Leaving RUNNING state. "
			<< fNumIterations
			<< " / " << fMaxIterations;
			return false;
		}
	}


#if 1
	static int counter = 0;
	counter++;
	int elapse = g_sw->elapse();
	if (elapse > 10*1000) {
		double freq = static_cast<double>(counter) / elapse;
 		LOG(info) << "processed events: " << fNumIterations;
		LOG(info) << "Freq. " << freq << " kHz, "
			<< "Q " << gQdepth;
		counter = 0;
		g_sw->restart();
	}
#endif

	return true;
}

#if 0
//_____________________________________________________________________________
bool Sampler::ConditionalRun()
{

	wait_trigger();


  auto text = new std::string(
	fConfig->GetProperty<std::string>("id")
	+ ":" + fText
	+ " : " + std::to_string(fNumIterations));
	gQdepth++;

  // copy
  auto txt = *text;

  FairMQMessagePtr msg(
    NewMessage(
      const_cast<char*>(text->data()),
      text->length(),
      [](void * /*data*/, void* object) { 
        auto p = reinterpret_cast<std::string*>(object);
        //LOG(debug) << " sent " << *p;
        delete p;
        gQdepth--;
      }, 
      text
    )
  );

  //LOG(info) << "Sending \"" << txt << "\"";

  if (Send(msg, "data") < 0) {
    LOG(warn) << " event:  " << fNumIterations;
    return false;
  } else { 
    ++fNumIterations;
    if (fMaxIterations > 0 && fNumIterations >= fMaxIterations) {
      LOG(info)
      << "Configured maximum number of iterations reached. Leaving RUNNING state. "
      << fNumIterations
      << " / " << fMaxIterations;
      return false;
    }
  }

  //LOG(info) << " processed events:  " << fNumIterations;

#if 1
	static int counter = 0;
	counter++;
	int elapse = g_sw->elapse();
	if (elapse > 10*1000) {
		double freq = static_cast<double>(counter) / elapse;
  		LOG(info) << "Sending \"" << txt << "\"";
		LOG(info) << "Freq. " << freq << " kHz, "
			<< "Q " << gQdepth;
		counter = 0;
		g_sw->restart();
	}
#endif

  return true;
}
#endif

//_____________________________________________________________________________
void Sampler::PostRun()
{
  LOG(debug) << __FUNCTION__;
  fNumIterations = 0;
}

//_____________________________________________________________________________
void Sampler::PreRun()
{
  LOG(debug) << __FUNCTION__;
}

//_____________________________________________________________________________
void Sampler::Run()
{
  LOG(debug) << __FUNCTION__;
}

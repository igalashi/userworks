/*
 *
 */

#include <iostream>
#include <string>
#include <sstream>

#include "uhbook.cxx"
#include "RedisDataStore.h"
#include "Slowdashify.h"
#include "UnpackRecbe.h"

constexpr int N_MODULE = 105;

static UH1Book *gId;
static UH1Book *gDipSwId;
static UH1Book *gTDCHIT[N_MODULE];
static UH1Book *gADCHIT[N_MODULE];

static UH1Book *gElapse;

static UH2Book *gTDC[N_MODULE];
static UH2Book *gADC[N_MODULE][Recbe::N_CH];
static UH2Book *gHit;

static RedisDataStore *gDB;
static std::string gDbURL;
static std::string gDevName;
//static int gDevNumber;
static std::string gNamePrefix;

void gHistInit()
{
	if (gId != nullptr) return;

	gId = new UH1Book("RecbeID", 110, 0, 110);
	gDipSwId = new UH1Book("Recbe Data ID", 110, 0, 110);

	for (int i = 0 ; i < N_MODULE ; i++) {
		std::ostringstream osst;
		osst << "TDC " << i;
		std::ostringstream ossa;
		ossa << "ADC " << i;
		gTDCHIT[i] = new UH1Book(osst.str().c_str(), 48, 0, 48);
		gADCHIT[i] = new UH1Book(ossa.str().c_str(), 48, 0, 48);
	}
	for (int i = 0 ; i < N_MODULE ; i++) {
		std::ostringstream osst;
		osst << "TDC " << i;
		gTDC[i] = new UH2Book(osst.str().c_str(), 100, 0, 1000, 48, 0, 48);
		for (int j = 0 ; j < Recbe::N_CH ; j++) {
			std::ostringstream ossa;
			ossa << "ADC Recbe: " << i << " Ch: " << j;
			gADC[i][j] = new UH2Book(ossa.str().c_str(), 32, 0, 32, 256, 0, 1024);
		}
	}

	gHit = new UH2Book("Hit channel", 110, 0, 110, 48, 0, 48);
	gElapse = new UH1Book("Elapse Time of DQM", 1000, 0, 1000);


	if (gDevName.size() < 3) {
		gDB = new RedisDataStore("tcp://127.0.0.1:6379/3");
	} else {
		gDB = new RedisDataStore(gDbURL);
	}

	return;
}

void gHistInit(std::string &url, std::string &name, int num)
{
	gDbURL = url;
	gDevName = name;
	//gDevNumber = num;

	std::ostringstream oss;
	oss << num;
	gNamePrefix = "dqm:" + name + "-" + oss.str();

	gHistInit();

	return;
}

void gHistInit(std::string &url, std::string &idname)
{
	gDbURL = url;
	gDevName = idname;
	//gDevNumber = 0;

	gNamePrefix = "dqm:" + idname;

	gHistInit();

	return;
}

void gHistDraw()
{

	#if 0
	gId->Print();
	gId->Draw();
	#endif

	std::cout << "+" << std::flush;

	std::string hhitname = gNamePrefix + ":" + "ModuleHitMap";
	gDB->write(hhitname.c_str(), Slowdashify(*gId));
	std::string hname = gNamePrefix + ":" + "ModuleHitMapDipSwId";
	gDB->write(hname.c_str(), Slowdashify(*gDipSwId));
	hname = gNamePrefix + ":" + "HitMap";
	gDB->write(hname.c_str(), Slowdashify(*gHit));

	static int counter = 0;
	if (counter++ == 16) counter = 0;

	for (int i = 0 ; i < N_MODULE ; i++) {
		if ((i % 16) == counter) {
			for (unsigned int j = 0 ; j < Recbe::N_CH ; j++) {
				std::ostringstream ossa;
				ossa << ":Recbe-" << std::setw(3) << std::setfill('0') << i
					<< ":ADC:Ch" << std::setw(2) << std::setfill('0') << j;
				hname = gNamePrefix + ossa.str();
				gDB->write(hname.c_str(), Slowdashify(*(gADC[i][j])));
	
				std::cout << "." << std::flush;
			}
		}
	}
	
	std::cout << "-" << std::flush;

	return;
}

void gHistReset()
{

	gId->Reset();
	gDipSwId->Reset();
	gHit->Reset();

	for (int i = 0 ; i < N_MODULE ; i++) {
		gTDCHIT[i]->Reset();
		gADCHIT[i]->Reset();

		gTDC[i]->Reset();
		for (int j = 0 ; j < Recbe::N_CH ; j++) gADC[i][j]->Reset();
	}

	return;
}


void gHistBook(fair::mq::MessagePtr& msg, int id, int type)
{

#if 1
	unsigned int msize = msg->GetSize();
	unsigned char *pdata = reinterpret_cast<unsigned char *>(msg->GetData());
#else
	(void)msg;
#endif
	static unsigned int prescale = 0;

	#if 0
	std::cout << "# " << std::setw(8) << j << " : "
		<< std::hex << std::setw(2) << std::setfill('0')
		<< std::setw(2) << static_cast<unsigned int>(pdata[0 + 7]) << " "
		<< std::setw(2) << static_cast<unsigned int>(pdata[0 + 6]) << " "
		<< std::setw(2) << static_cast<unsigned int>(pdata[0 + 5]) << " "
		<< std::setw(2) << static_cast<unsigned int>(pdata[0 + 4]) << " "
		<< std::setw(2) << static_cast<unsigned int>(pdata[0 + 3]) << " "
		<< std::setw(2) << static_cast<unsigned int>(pdata[0 + 2]) << " "
		<< std::setw(2) << static_cast<unsigned int>(pdata[0 + 1]) << " "
		<< std::setw(2) << static_cast<unsigned int>(pdata[0 + 0]) << " : ";
	#endif

	#if 0
	uint64_t msg_magic = *(reinterpret_cast<uint64_t *>(pdata));
	std::cout << "#Unknown Header " << std::hex << msg_magic << std::endl;
	std::cout << "#FE id: " << id <<" type: " << type << std::endl;
	#else
	(void)type;
	#endif

	gId->Fill(id + 0.5);

	struct Recbe::Data recbe;
	Recbe::Unpack(reinterpret_cast<char *>(msg->GetData()), recbe);

	std::vector<bool> hit;
	hit.resize(Recbe::N_CH);
	for (int i = 0 ; i < Recbe::N_CH ; i++) hit[i] = false;

	gDipSwId->Fill(recbe.Id);
	for (int ch = 0 ; ch < Recbe::N_CH ; ch++) {
		for (unsigned int i = 0 ; i < recbe.Tdc[ch].size() ; i++) {
			if (recbe.Tdc[ch][i].Hit == true) {
				gHit->Fill(recbe.Id, ch, 1.);
				hit[i] = true;
			}
		}
	}

	for (int ch = 0 ; ch < Recbe::N_CH ; ch++) {
		if (hit[ch]) {
			//if (recbe.Id == 5) std::cout << "Recbe: " << recbe.Id << " Ch: " << ch << " size: " << recbe.Adc[ch].size() << std::endl;
			for (unsigned int j = 0 ; j < recbe.Adc[ch].size() ; j++) {
				gADC[recbe.Id][ch]->Fill(
					static_cast<double>(j),
				       	static_cast<double>(recbe.Adc[ch][j]), 1.);

				//std::cout
				//	<< " " << static_cast<double>(j)
				//	<< ":" << static_cast<double>(recbe.Adc[ch][j]);
				//gADC[recbe.Id][ch]->Print();
			}
			//if (recbe.Id == 5) gADC[recbe.Id][ch]->Print();
		}
	}


	uint16_t *recbe_data = reinterpret_cast<uint16_t *>(pdata + sizeof(struct Recbe::Header));
	unsigned int body_size = (msize - sizeof(struct Recbe::Header)) / sizeof(uint16_t);
	if (id == 0) {
		int n = 0;
		unsigned int i = 0;
		while (i < body_size) {
			//std::cout << "#D N: " << n << " : " << " size: " << body_size << std::endl;

			//std::cout << "#D ADC : "; 
			//for (int j = 0 ; j < 48 ; j++) std:: cout << recbe_data[i++] << " ";
			//std::cout << std::endl;
	
			#if 0
			std::cout << "#D ADC: "; 
			for (int j = 0 ; j < 48 ; j++) {
				unsigned int val
					= ((recbe_data[i] & 0x00ff) << 8)
					| ((recbe_data[i] & 0xff00) >> 8);
				std:: cout << val << " ";
				i++;
			}
			std::cout << std::endl;
			#endif

			//std::cout << "#D TDC: "; 
			for (int j = 0 ; j < 48 ; j++) {
				unsigned int val
					= ((recbe_data[i] & 0x00ff) << 8)
					| ((recbe_data[i] & 0xff00) >> 8);
				//if (val > 0) std:: cout << j << ":" << val << " ";
				if (val > 0) std:: cout << "ID: " << id << " N: " << n << " TDC: " << j << ":" << val << " " << std::endl;
				i++;
			}
			//std::cout << std::endl;
			n++;
		}
	}

#if 0
	unsigned int i = 0;
	int tic = 0;
	while (i < body_size) {
		//std::cout << "ADC[" << tic << "]";
		for (int j = 0 ; j < 48 ; j++) {
			int val = ntohs(
				*(reinterpret_cast<uint16_t *>(
					(recbe_data + ((tic * 48 * 2) + j) * sizeof(uint16_t))
				)));
			//std::cout << " " << val;
			if (val > 0xd8) gADCHIT[id]->Fill(j);

			i += sizeof(uint16_t);
		}
		//std::cout << std::endl;
		//std::cout << "TDC[" << tic << "]";
		for (int j = 0 ; j < 48 ; j++) {
			unsigned int bufaddress = ((tic * 48 * 2) + 48 + j) * sizeof(uint16_t);
			if (bufaddress > msize) {
				std::cout << "#E buffer exceed. "
					<< bufaddress << " / " << msize
					<< " index: " << tic << " " << j << " i: " << i
					<< std::endl;
				break;
			} else {
				int val = ntohs(
					*(reinterpret_cast<uint16_t *>(
						(recbe_data + ((tic * 48 * 2 + 48) + j) * sizeof(uint16_t))
					)));
				// if (val > 0) std::cout << " " << val;
				if (val > 0) gTDCHIT[id]->Fill(j);
			}

			i += sizeof(uint16_t);
		}
		//std::cout << std::endl;
		tic++;
	}
#endif

	prescale++;
	return ;
}

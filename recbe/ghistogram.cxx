/*
 *
 */

#include <iostream>
#include <string>
#include <sstream>

#include "uhbook.cxx"
#include "RedisDataStore.h"
#include "Slowdashify.h"

constexpr int N_MODULE = 105;

static UH1Book *gID;
static UH1Book *gTDCHIT[N_MODULE];
static UH1Book *gADCHIT[N_MODULE];

static UH1Book *gTDC01[48];
static UH1Book *gADC01[48];

static RedisDataStore *gDB;
static std::string gDBURL;
static std::string gDevName;
//static int gDevNumber;
static std::string gNamePrefix;

void gHistInit()
{

	gID = new UH1Book("RecbeID", 105, 0, 105);
	for (int i = 0 ; i < N_MODULE ; i++) {
		std::ostringstream osst("TDC ");
		osst << i;
		std::ostringstream ossa("ADC ");
		ossa << i;
		gTDCHIT[i] = new UH1Book(osst.str().c_str(), 48, 0, 48);
		gADCHIT[i] = new UH1Book(ossa.str().c_str(), 48, 0, 48);
	}
	for (int i = 0 ; i < 48 ; i++) {
		std::ostringstream osst("TDC CH ");
		osst << i;
		std::ostringstream ossa("ADC CH ");
		ossa << i;
		gTDC01[i] = new UH1Book(osst.str().c_str(), 100, 0, 1000);
		gADC01[i] = new UH1Book(ossa.str().c_str(), 100, 0, 1000);
	}

	if (gDevName.size() < 3) {
		gDB = new RedisDataStore("tcp://127.0.0.1:6379/3");
	} else {
		gDB = new RedisDataStore(gDBURL);
	}

	return;
}

void gHistInit(std::string &url, std::string &name, int num)
{
	gDBURL = url;
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
	gDBURL = url;
	gDevName = idname;
	//gDevNumber = 0;

	gNamePrefix = "dqm:" + idname;

	gHistInit();

	return;
}

void gHistDraw()
{

	#if 0
	gID->Print();
	gID->Draw();
	#endif

	std::string hhitname = gNamePrefix + ":" + "HitMap";
	gDB->write(hhitname.c_str(), Slowdashify(*gID));
	//std::cout << "#D HitMap: " << Slowdashify(*gID) << std::endl;
	
	return;
}

void gHistReset()
{

	gID->Reset();
	for (int i = 0 ; i < N_MODULE ; i++) {
		gTDCHIT[i]->Reset();
		gADCHIT[i]->Reset();
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

	gID->Fill(id + 0.5);

	//if (id < 10) std::cout << "#ID " << id << " ";

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
	
			//std::cout << "#D ADC: "; 
			for (int j = 0 ; j < 48 ; j++) {
				unsigned int val
					= ((recbe_data[i] & 0x00ff) << 8)
					| ((recbe_data[i] & 0xff00) >> 8);
			//	std:: cout << val << " ";
				i++;
			}
			//std::cout << std::endl;

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

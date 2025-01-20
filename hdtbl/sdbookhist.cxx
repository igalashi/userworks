/*
 *
 */

#include <iostream>
#include <string>
#include <sstream>

#include "uhbook.cxx"
#include "RedisDataStore.h"
#include "Slowdashify.h"

#include "FilterHeader.h"


static UH1Book *gHHRTDC[2];
static UH2Book *gH2HRTDC;
static UH1Book *gHElapse;

static RedisDataStore *gDB;
static std::string gDbURL;
static std::string gDevName;
static std::string gNamePrefix;


//TMutex *gMtxDisp;

#if 0
void gEventCycle(void *arg = NULL)
{
	(void)arg;

	while (true) {
		//gMtxDisp->Lock();
		gSystem->ProcessEvents();
		//gMtxDisp->UnLock();
		TThread::Sleep(0, 100'000'000);
	}
	return;
}
#endif

void gProcessEvent()
{
#if 0
	//gMtxDisp->Lock();
	gSystem->ProcessEvents();
	//gMtxDisp->UnLock();
#endif
	return;
}

void gHistInit()
{
	gDbURL = "tcp://127.0.0.1:6379/3";
	gDevName = "hdtbldisplay";
	gNamePrefix = "dqm:" + gDevName;

        if (gDevName.size() < 3) {
                gDB = new RedisDataStore("tcp://127.0.0.1:6379/3");
        } else {
                gDB = new RedisDataStore(gDbURL);
        }

	gHHRTDC[0] = new UH1Book("HR-TDC0", 64, 0, 64);
	gHHRTDC[1] = new UH1Book("HR-TDC1", 64, 0, 64);
	gH2HRTDC = new UH2Book("HR-TDC 2D", 64, 0, 64, 64, 0, 64);
        gHElapse = new UH1Book("Elapse Time (ms)", 500, 0, 20000);
 
	return;
}

void gHistDraw()
{

#if 0
	for (int i = 0 ; i < 4 ; i++) gCan1->cd(i + 1)->Modified();

	//gMtxDisp->Lock();
	gCan1->Update();
	//gMtxDisp->UnLock();
#endif

	std::string htdc1name = gNamePrefix + ":" + "TDC0";
	gDB->write(htdc1name.c_str(), Slowdashify(*gHHRTDC[0]));
	std::string htdc2name = gNamePrefix + ":" + "TDC1";
	gDB->write(htdc2name.c_str(), Slowdashify(*gHHRTDC[1]));
	std::string helapsename = gNamePrefix + ":" + "Elapse";
	gDB->write(helapsename.c_str(), Slowdashify(*gHElapse));

	return;
}

void gHistReset()
{
	//gMtxDisp->Lock();

	gHHRTDC[0]->Reset();
	gHHRTDC[1]->Reset();
	gH2HRTDC->Reset();
	gHElapse->Reset();

	//gMtxDisp->UnLock();

	return;
}

void gHistFlt(struct Filter::Header *pflt)
{

	gHElapse->Fill(static_cast<double>(pflt->elapseTime));

	return;
}

void gHistBook(fair::mq::MessagePtr& msg, uint32_t id, int type)
{
	unsigned int msize = msg->GetSize();
	unsigned char *pdata = reinterpret_cast<unsigned char *>(msg->GetData());
	//uint64_t msg_magic = *(reinterpret_cast<uint64_t *>(pdata));

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
	//std::cout << " " << (id & 0xff);

	for (size_t i = 0 ; i < msize ; i += sizeof(uint64_t)) {
		if ((id & 0x000000ff) == 16) {
			if ((pdata[i + 7] & 0xfc) == (TDC64H_V3::T_TDC << 2)) {

				uint64_t *dword = reinterpret_cast<uint64_t *>(&(pdata[i]));
				if (type == SubTimeFrame::TDC64H_V3) {
					struct TDC64H_V3::tdc64 tdc;
					TDC64H_V3::Unpack(*dword, &tdc);
					//gMtxDisp->Lock();
					gHHRTDC[0]->Fill(tdc.ch);
					//gMtxDisp->UnLock();
#if 0
					std::cout << "FEM: " << (id & 0xff) << " TDC ";
					std::cout << "H :"
					          << " CH: " << std::dec << std::setw(3) << tdc.ch
					          << " TDC: " << std::setw(7) << tdc.tdc << std::endl;
#else
					//std::cout << "*" << std::flush;
#endif

				} else if (type == SubTimeFrame::TDC64L_V3) {
					struct TDC64L_V3::tdc64 tdc;
					TDC64L_V3::Unpack(*dword, &tdc);
#if 0
					std::cout << "FEM: " << (id & 0xff) << " TDC ";
					std::cout << "L :"
					          << " CH: " << std::dec << std::setw(3) << tdc.ch
					          << " TDC: " << std::setw(7) << tdc.tdc << std::endl;
#endif
				} else {
					std::cout << "UNKNOWN"<< std::endl;
				}
			}
		}

		if ((id & 0x000000ff) == 17) {
			if ((pdata[i + 7] & 0xfc) == (TDC64H_V3::T_TDC << 2)) {

				uint64_t *dword = reinterpret_cast<uint64_t *>(&(pdata[i]));
				if (type == SubTimeFrame::TDC64H_V3) {
					struct TDC64H_V3::tdc64 tdc;
					TDC64H_V3::Unpack(*dword, &tdc);
					//gMtxDisp->Lock();
					gHHRTDC[1]->Fill(tdc.ch);
					//gMtxDisp->UnLock();
#if 0
					std::cout << "FEM: " << (id & 0xff) << " TDC ";
					std::cout << "H :"
					          << " CH: " << std::dec << std::setw(3) << tdc.ch
					          << " TDC: " << std::setw(7) << tdc.tdc << std::endl;
#endif
				}
			}
		}

	}

	// Dummy
	static std::mt19937_64 mt64(0);
	std::uniform_int_distribution<uint64_t> get_rand_uni_int(0, 64);
	int ch = get_rand_uni_int(mt64);
	gHHRTDC[1]->Fill(ch);
	//std::cout << "+" << std::flush;


	prescale++;

	return ;
}

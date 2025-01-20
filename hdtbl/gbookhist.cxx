/*
 *
 */

#include "TCanvas.h"
#include "TThread.h"
#include "TMutex.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TSystem.h"
#include "TApplication.h"

#include "FilterHeader.h"

static TApplication *gApp = new TApplication("App", nullptr, nullptr);

static TCanvas *gCan1 = new TCanvas ("DISPLAY", "Display");

static TH1F *gHHRTDC[2];
static TH2F *gH2HRTDC;
static TH1F *gHElapse;


//TMutex *gMtxDisp;

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

void gProcessEvent()
{
	//gMtxDisp->Lock();
	gSystem->ProcessEvents();
	//gMtxDisp->UnLock();
	return;
}

void gHistInit()
{
	//gMtxDisp = new TMutex();

	gHHRTDC[0] = new TH1F("HRTDC0", "HR-TDC0", 64, 0, 64);
	gHHRTDC[1] = new TH1F("HRTDC1", "HR-TDC1", 64, 0, 64);
	gH2HRTDC = new TH2F("HRTDC02D", "HR-TDC 2D", 64, 0, 64, 64, 0, 64);
        gHElapse = new TH1F("ELAPSE", "Elapse Time (ms)", 500, 0, 20000);
 

	gCan1->Divide(2, 2);
	gCan1->cd(1);
	gHHRTDC[0]->Draw();
	gCan1->cd(2);
	gHHRTDC[1]->Draw();
	gCan1->cd(3);
	gH2HRTDC->Draw();
	gCan1->cd(4);
	gHElapse->Draw();


	return;
}

void gHistDraw()
{

	for (int i = 0 ; i < 4 ; i++) gCan1->cd(i + 1)->Modified();

	//gMtxDisp->Lock();
	gCan1->Update();
	//gMtxDisp->UnLock();

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

	gHElapse->Fill(pflt->elapseTime);

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

	prescale++;

	return ;
}

/*
 *
 */

#include "TCanvas.h"
#include "TThread.h"
#include "TMutex.h"
#include "TH1F.h"
#include "TH2F.h"

#include "FilterHeader.h"

static TCanvas *gCan1 = new TCanvas ("DISPLAY", "Display");
static TCanvas *gCan2 = new TCanvas ("DISPLAY2", "Display2");
static TH1F *gHHRTDC[2];
static TH2F *gH2HRTDC;
static TH1F *gHDiff;

static TH1F *gHTrig;
static TH2F *gH2TrigWindow;
static TH2F *gH2TrigWindow_one;
static TH1F *gHElapse;
static TH1F *gHNTrig;

static TH1F *gHBDC[2];
static TH1F *gHSFT[3];
static TH1F *gHKLDC[2];


static std::vector< std::vector<int> > gIpBDC = {
	{161, 162, 163, 164},
	{166, 166, 166, 168}};
static std::vector<int> gIpSFT = {173, 174, 175};
static std::vector< std::vector<int> > gIpKLDC = {
	{173, 174}, {175, 176}};

static std::vector< std::vector<int> > gTrig;

struct entry_tdc {
	std::vector< std::vector<int> > tdc4n;
	int      index;
	uint32_t module;
	int      channel;
	int      offset;
	int      type;
};
static std::vector<struct entry_tdc> gEntryTDC;

struct signal_id {
	int      index;
	uint32_t module;
	int      channel;
	int      offset;
};

static std::vector<struct signal_id> trg_sources = {
	{ 0, 0xc0a802a9,  8, 0}, { 1, 0xc0a802a9, 10, 0},
	{ 2, 0xc0a802aa, 16, -12}, { 3, 0xc0a802aa, 17, -12}, { 4, 0xc0a802aa, 18, -12}, { 5, 0xc0a802aa, 19, -12},
	{ 6, 0xc0a802aa, 20, -12}, { 7, 0xc0a802aa, 21, -12}, { 8, 0xc0a802aa, 22, -12}, { 9, 0xc0a802aa, 23, -12},
	{10, 0xc0a802aa, 24, -12}, {11, 0xc0a802aa, 25, -12}, {12, 0xc0a802aa, 27, -12}, {13, 0xc0a802aa, 28, -12}
};



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

void gHistInit()
{
	//gMtxDisp = new TMutex();

	gHHRTDC[0] = new TH1F("HRTDC0", "HR-TDC0", 64, 0, 64);
	gHHRTDC[1] = new TH1F("HRTDC1", "HR-TDC1", 64, 0, 64);
	gH2HRTDC = new TH2F("HRTDC02D", "HR-TDC 2D", 64, 0, 64, 64, 0, 64);
	gHDiff = new TH1F("DIFF", "Diff", 1000, 0, 1000000);

	gHTrig = new TH1F("TRIG", "Trigger", 1000, 0, 150000);
	gH2TrigWindow = new TH2F("TWINDOW", "Trigger Window", 1000, -500., 500., 32, 0., 32.);
	gH2TrigWindow_one = new TH2F("TWONE", "Trigger Window One Event", 1000, -500., 500., 32, 0., 32.);
	gHElapse = new TH1F("ELAPSE", "Elapse Time (ms)", 500, 0, 20000);
	gHNTrig = new TH1F("NTRIG", "Number of Trigger (/STF)", 500, 0, 1000);


	gHBDC[0] = new TH1F("BDC1", "BDC1", 512, 0, 512);
	gHBDC[1] = new TH1F("BDC2", "BDC2", 512, 0, 512);
	gHSFT[0] = new TH1F("SFT1", "SFT", 128, 0, 128);
	gHSFT[1] = new TH1F("SFT2", "SFT", 128, 0, 128);
	gHSFT[2] = new TH1F("SFT3", "SFT", 128, 0, 128);
	gHKLDC[0] = new TH1F("KLDC1", "KLDC1", 256, 0, 256);
	gHKLDC[1] = new TH1F("KLDC2", "KLDC2", 256, 0, 256);


	gCan1->Divide(2, 2);
	//gCan1->cd(1)->SetLogy();
	//gCan1->cd(1); gHHRTDC[0]->Draw();
	//gCan1->cd(2)->SetLogy();
	//gCan1->cd(2); gHHRTDC[1]->Draw();

	//gCan1->cd(3)->SetLogz();
	//gCan1->cd(3); gH2HRTDC->Draw("col2");
	//gCan1->cd(4); gHDiff->Draw();

	gCan1->cd(1); gHElapse->Draw();
	gHNTrig->GetXaxis()->SetRangeUser(2., 1000.);
	gCan1->cd(2); gHNTrig->Draw();
	//gCan1->cd(3); gHTrig->Draw();

	gH2TrigWindow_one->GetXaxis()->SetRangeUser(-100., 100.);
	gH2TrigWindow_one->GetYaxis()->SetRangeUser(0., 16.);
	gCan1->cd(3); gH2TrigWindow_one->Draw("box");

	gH2TrigWindow->GetXaxis()->SetRangeUser(-100., 100.);
	gH2TrigWindow->GetYaxis()->SetRangeUser(0., 16.);
	gCan1->cd(4)->SetLogz();
	gCan1->cd(4); gH2TrigWindow->Draw("col2");


	gCan2->Divide(2, 5);
	gCan2->cd(1)->SetLogy();
	gCan2->cd(1); gHHRTDC[0]->Draw();
	gCan2->cd(2)->SetLogy();
	gCan2->cd(2); gHHRTDC[1]->Draw();

	gCan2->cd(3); gHBDC[0]->Draw();
	gCan2->cd(4); gHBDC[1]->Draw();
	gCan2->cd(5); gHSFT[0]->Draw();
	gCan2->cd(6); gHSFT[1]->Draw();
	gCan2->cd(7); gHSFT[2]->Draw();
	gCan2->cd(9); gHKLDC[0]->Draw();
	gCan2->cd(10); gHKLDC[1]->Draw();

	return;
}

void gHistDraw()
{

	for (int i = 0 ; i < 4 ; i++) gCan1->cd(i + 1)->Modified();
	for (int i = 0 ; i < 7 ; i++) gCan2->cd(i + 1)->Modified();
	for (int i = 8 ; i < 10 ; i++) gCan2->cd(i + 1)->Modified();

	//gMtxDisp->Lock();
	gCan1->Update();
	gCan2->Update();
	//gMtxDisp->UnLock();
	
	return;
}

void gHistReset()
{
	//gMtxDisp->Lock();

	gHHRTDC[0]->Reset();
	gHHRTDC[1]->Reset();
	gH2HRTDC->Reset();
	gHDiff->Reset();

	gHTrig->Reset();
	gH2TrigWindow->Reset();
	gH2TrigWindow_one->Reset();
	gHElapse->Reset();
	gHNTrig->Reset();

	gHBDC[0]->Reset();
	gHBDC[1]->Reset();
	gHSFT[0]->Reset();
	gHSFT[1]->Reset();
	gHSFT[2]->Reset();
	gHKLDC[0]->Reset();
	gHKLDC[1]->Reset();

	//gMtxDisp->UnLock();

	return;
}

void gHistFlt(struct Filter::Header *pflt)
{
	
	gHElapse->Fill(pflt->elapseTime);
	gHNTrig->Fill(pflt->numTrigs);

	return;
}

void gHistTrig_init()
{
	for (auto &i : trg_sources) {
		struct entry_tdc src;
		src.index = i.index;
		src.module = i.module;
		src.channel = i.channel;
		src.offset = i.offset;
		src.tdc4n.clear();
		src.tdc4n.resize(0);
		gEntryTDC.emplace_back(src);
	}

	return;
}

bool gTrig_isvalid = false;
void gHistTrig_clear()
{
	gTrig.clear();
	gTrig.resize(0);
	gTrig_isvalid = false;

	// gEntryTDC.clear();
	// gEntryTDC.resize(0);
	for (auto &i : gEntryTDC) {
		i.tdc4n.clear();
		i.tdc4n.resize(0);
	}
	
	return;
}

void gHistTrig(Filter::TrgTime *pdata, int len)
{
	//std::cout << "#D gHistTrig ";

	std::vector<int> trig_in_hbf;

	for (int i = 0 ; i < len ; i++) {
		Filter::TrgTime *t = reinterpret_cast<Filter::TrgTime *>(pdata + i);
		if (t->type == 0xaa000000) trig_in_hbf.emplace_back(t->time);
		gHTrig->Fill(t->time);
		//std::cout << "type: " << t->type << " time: " << t->time;
	}
	gTrig.emplace_back(trig_in_hbf);

	//std::cout << std::endl;
	gTrig_isvalid = true;

	//std::cout << "gHistTrig: " << gTrig.size() << std::endl;

	return;
}

void gHistEntryTDC(fair::mq::MessagePtr& msg, uint32_t id, int type)
{
	//unsigned int msize = msg->GetSize();
	//uint64_t msg_magic = *(reinterpret_cast<uint64_t *>(msg->GetData()));
	//HartbeatFrame::Header *phbf = reinterpret_cast<HartbeatFrame::Header *>(msg->GetData());

	unsigned char *pdata = reinterpret_cast<unsigned char *>(
		reinterpret_cast<unsigned char *>(msg->GetData()) + sizeof(HartbeatFrame::Header));
	unsigned int dsize = msg->GetSize() - sizeof(HartbeatFrame::Header);

	for (auto &mod : gEntryTDC) {
		if (id == mod.module) {
			mod.type = type;
			std::vector<int> tdc4n;
			for (size_t i = 0 ; i < dsize ; i += sizeof(uint64_t)) {

				uint64_t *dword = reinterpret_cast<uint64_t *>(&(pdata[i]));
				struct TDC64H_V3::tdc64 tdc;
				TDC64H_V3::Unpack(*dword, &tdc);

				if (tdc.ch == mod.channel) {
					//std::cout << "#D " << tdc.ch << " : " << tdc.tdc << std::endl;
					tdc4n.emplace_back(tdc.tdc4n);
				}

			}

			mod.tdc4n.emplace_back(tdc4n);
		}
	}

	return;
}

void gHistBookTrigWin()
{
	static unsigned int tw_counter = 0;
	//static unsigned int tw_nhit = 0;
	const unsigned int tw_pre_factor = 1000;	

	for (unsigned int i = 0 ; i < gTrig.size() ; i++) {
		for (auto &trg : gTrig[i]) {
			bool is_draw = ((tw_counter++ % tw_pre_factor) == 0);
			if (is_draw) gH2TrigWindow_one->Reset();
			for (auto &mod : gEntryTDC) {
				for (auto &tdc4n : mod.tdc4n[i]) {
					if (is_draw) {
						int diff = tdc4n - trg;
						if (std::abs(diff) < 500) {
							gH2TrigWindow_one->Fill(diff + mod.offset, mod.index);
							//tw_nhit++;
							//std::cout << "*" << std::flush;
						}
					}
				}

			}
			if (is_draw) gCan1->cd(3)->Modified();
		}
	}
	
	return;
}

void gHistBookTrigWin(fair::mq::MessagePtr& msg, uint32_t id, int type)
{
	unsigned int msize = msg->GetSize();
	unsigned char *pdata = reinterpret_cast<unsigned char *>(msg->GetData());

	static unsigned int tw_counter = 0;
	static unsigned int tw_nhit = 0;
	const unsigned int tw_pre_factor = 10000;	

	if (gTrig_isvalid && (type == SubTimeFrame::TDC64H_V3)) {
	for (auto &trigs : gTrig) {
	for (auto &trg : trigs) {
		bool tw_one = ((tw_counter++ % tw_pre_factor) == 0);
		if ((tw_one) && (tw_nhit > 0)) {
			gH2TrigWindow_one->Reset();
			std::cout << tw_nhit << ":" << tw_counter << std::endl;
			tw_nhit = 0;
		}
		for (size_t i = 0 ; i < msize ; i += sizeof(uint64_t)) {
			for (auto &sig : trg_sources) {
				if ((pdata[i + 7] & 0xfc) == (TDC64H_V3::T_TDC << 2) 
					&& (type == SubTimeFrame::TDC64H_V3)) {

					uint64_t *dword = reinterpret_cast<uint64_t *>(&(pdata[i]));
					struct TDC64H_V3::tdc64 tdc;
					TDC64H_V3::Unpack(*dword, &tdc);

					if ((id == sig.module) && (tdc.ch == sig.channel)) {

						#if 0
						if(gTrig.size() > 0) {
							std::cout << "#D Module :" << id
							<< " CH: " << std::dec << std::setw(3) << tdc.ch
							<< " TDC: " << std::setw(7) << tdc.tdc
							<< " Trig[0]: " << gTrig[0] << std::endl;
						}
						#endif

						int diff = tdc.tdc4n - trg;
						if (std::abs(diff) < 500) {
							gH2TrigWindow->Fill(diff + sig.offset, sig.index);
						}

						#if 0
						if (tw_one && (std::abs(diff) < 500)) {
							gH2TrigWindow_one->Fill(diff + sig.offset, sig.index);

							gCan1->cd(3)->Modified();

							std::cout << "." << std::flush;
							tw_nhit++;
						}
						#endif
					}
				}
			}
		}
		if (tw_one) std::cout << std::endl;
	}
	}
	}

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
		if ((id & 0x000000ff) == 169) {
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

				} else
				if (type == SubTimeFrame::TDC64L_V3) {
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

		if ((id & 0x000000ff) == 170) {
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


		#if 0
		static unsigned int tw_prescale = 0;
		bool tw_one = false;
		int tw_nhit = 0;
		if ((tw_prescale++ % 1000) == 0)  {
			if (gTrig.size() > 0) {
				tw_one = true;
				double stats[8];
				//gH2TrigWindow_one->GetStats(stats);
				//for (int ii = 0 ; ii < 8 ; ii++) std::cout << " " << stats[ii];
				if (tw_nhit > 0) {
					gH2TrigWindow_one->Reset();
					tw_nhit = 0;
				}
				//std::cout << ":" << stats[0] << " " << gTrig.size() << std::flush;
			} else {
				tw_one = false;
			}
		} else {
			tw_one = false;
		}
		#endif

		for (auto &sig : trg_sources) {

			if ((pdata[i + 7] & 0xfc) == (TDC64H_V3::T_TDC << 2) 
				&& (type == SubTimeFrame::TDC64H_V3)) {

				uint64_t *dword = reinterpret_cast<uint64_t *>(&(pdata[i]));
				struct TDC64H_V3::tdc64 tdc;
				TDC64H_V3::Unpack(*dword, &tdc);

				if ((id == sig.module) && (tdc.ch == sig.channel)) {

					#if 0
					if(gTrig.size() > 0) {
						std::cout << "#D Module :" << id
						<< " CH: " << std::dec << std::setw(3) << tdc.ch
						<< " TDC: " << std::setw(7) << tdc.tdc
						<< " Trig[0]: " << gTrig[0] << std::endl;
					}
					#endif
	
					for (auto &trigs : gTrig) {
					for (auto &trg : trigs) {
						int diff = tdc.tdc4n - trg;
						if (std::abs(diff) < 1000) {
							gH2TrigWindow->Fill(diff + sig.offset, sig.index);
						}

						#if 0
						if (tw_one && (std::abs(diff) < 100)) {
							gH2TrigWindow_one->Fill(diff + sig.offset, sig.index);
							std::cout << "." << std::flush;
							tw_nhit++;
						}
						#endif
					}
					}
				}
			}
		}


	}

	uint64_t *pdata64 = reinterpret_cast<uint64_t *>(msg->GetData());
	if ((id & 0x000000ff) == 169) {
		for (size_t ii = 0 ; ii < (msize / sizeof(uint64_t)) ; ii++) {
			if (((pdata64[ii] & 0xfc00'0000'0000'0000) >> 58) == TDC64H_V3::T_TDC) {
				struct TDC64H_V3::tdc64 tdc;
				TDC64H_V3::Unpack(pdata64[ii], &tdc);
				auto chx = tdc.ch;
				auto tdcx = tdc.tdc;
				for (size_t j = 0 ; j < (msize / sizeof(uint64_t)) ; j++) {
					if (((pdata64[j] & 0xfc00'0000'0000'0000) >> 58) == TDC64H_V3::T_TDC) {
						TDC64H_V3::Unpack(pdata64[j], &tdc);
						auto chy = tdc.ch;
						auto tdcy = tdc.tdc;

						//std::cout << " " << abs(tdcx - tdcy);
						int diff = abs(tdcx - tdcy);
						if (diff > 0) {
							//gMtxDisp->Lock();
							gHDiff->Fill(diff);
							//gMtxDisp->UnLock();
							if (diff < 50'000) {
								if (chx == chy) {
									//std::cout << "#D " << chx
									//	<< " " << diff << std::endl;
								} else {
									//gMtxDisp->Lock();
									gH2HRTDC->Fill(chx, chy);
									//gMtxDisp->UnLock();
								}
							}
						}
					}
				}
			}
		}
	}


	if ((prescale % 10) == 0) {

	int offset = 0;
	TH1F *hist;
	bool isBook = false;
	switch (id & 0x000000ff) {
		case 161 : hist = gHBDC[0] ; offset =   0; isBook = true; break;
		case 162 : hist = gHBDC[0] ; offset = 128; isBook = true; break;
		case 163 : hist = gHBDC[0] ; offset = 256; isBook = true; break;
		case 164 : hist = gHBDC[0] ; offset = 384; isBook = true; break;

		case 165 : hist = gHBDC[1] ; offset =   0; isBook = true; break;
		case 166 : hist = gHBDC[1] ; offset = 128; isBook = true; break;
		case 167 : hist = gHBDC[1] ; offset = 256; isBook = true; break;
		case 168 : hist = gHBDC[1] ; offset = 384; isBook = true; break;

		case 173 : hist = gHSFT[0] ; offset =   0; isBook = true; break;
		case 174 : hist = gHSFT[1] ; offset =   0; isBook = true; break;
		case 175 : hist = gHSFT[2] ; offset =   0; isBook = true; break;

		case 176 : hist = gHKLDC[0] ; offset =   0; isBook = true; break;
		case 177 : hist = gHKLDC[0] ; offset = 128; isBook = true; break;
		case 178 : hist = gHKLDC[1] ; offset =   0; isBook = true; break;
		case 179 : hist = gHKLDC[1] ; offset = 128; isBook = true; break;

		default : isBook = false; break;
	}
	//std::cout << "#D id:" << (id & 0x000000ff) <<  " " << isBook << std::endl;

	if (isBook) {
		for (size_t ii = 0 ; ii < (msize / sizeof(uint64_t)) ; ii++) {
			if (((pdata64[ii] & 0xfc00'0000'0000'0000) >> 58) == TDC64L_V3::T_TDC) {
				if (type == SubTimeFrame::TDC64L_V3) {
					struct TDC64L_V3::tdc64 tdc;
					TDC64L_V3::Unpack(pdata64[ii], &tdc);
					//gMtxDisp->Lock();
					hist->Fill(tdc.ch + offset);
					//gMtxDisp->UnLock();
				}
			}
		}
	}

	}

	prescale++;

	return;
}


/********************************************************************************
 *                                                                              *
 ********************************************************************************/

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>

#include "uhbook.cxx"
#include "RedisDataStore.h"
#include "Slowdashify.h"


static const int NUM = 100;
static UH1Book *gH1hist[NUM];
static UH2Book *gH2hist[NUM];

static RedisDataStore *gDB;
static std::string gDbURL;
static std::string gDevName;
static std::string gNamePrefix;

void gHistInit(int size, char *name)
{
	gDbURL = "tcp://127.0.0.1:6379/3";
	gDevName = name;
	gNamePrefix = "dqm:" + gDevName;

	if (gDbURL.size() < 3) {
		gDB = new RedisDataStore("tcp://127.0.0.1:6379/3");
	} else {
		gDB = new RedisDataStore(gDbURL);
	}

	for (int i = 0 ; i < NUM ; i++) {
		if (gH1hist[i] != nullptr) delete gH1hist[i];
		if (gH2hist[i] != nullptr) delete gH2hist[i];
	}

	for (int i = 0 ; i < NUM ; i++) {
		std::stringstream ss1;
		ss1 << "1D-Hist "  << std::setw(4) << std::setfill('0') << i;
		std::stringstream ss2;
		ss2  << "2D-Hist " << std::setw(4) << std::setfill('0') << i;
		
		gH1hist[i] = new UH1Book(ss1.str().c_str(), size, 0, size);
		gH1hist[i]->SetXLabel("Arbitrary");
		gH1hist[i]->SetYLabel("Counts");
		gH2hist[i] = new UH2Book(ss2.str().c_str(), size, 0, size, size, 0, size);
	}

	return;
}


void gHistDraw()
{

	for (int i = 0 ; i < NUM ; i++) {
		std::stringstream ss1;
		ss1  << gNamePrefix << ":Hist1-" << std::setw(4) << std::setfill('0') << i;
		std::stringstream ss2;
		ss2  << gNamePrefix << ":Hist2-" << std::setw(4) << std::setfill('0') << i;
		
		gDB->write(ss1.str().c_str(), Slowdashify(*gH1hist[i]));
		gDB->write(ss2.str().c_str(), Slowdashify(*gH2hist[i]));
	}

	return;
}


void gHist1Draw()
{
	for (int i = 0 ; i < NUM ; i++) {
		std::stringstream ss1;
		ss1  << gNamePrefix << ":Hist1-" << std::setw(4) << std::setfill('0') << i;
		gDB->write(ss1.str().c_str(), Slowdashify(*gH1hist[i]));
	}

	return;
}

void gHist2Draw()
{
	for (int i = 0 ; i < NUM ; i++) {
		std::stringstream ss2;
		ss2  << gNamePrefix << ":Hist2-" << std::setw(4) << std::setfill('0') << i;
		gDB->write(ss2.str().c_str(), Slowdashify(*gH2hist[i]));
	}

	return;
}


void gHistReset()
{
	for (int i = 0 ; i < NUM ; i++) {
		gH1hist[i]->Reset();
		gH2hist[i]->Reset();
	}

	return;
}

void gHistBook(int size)
{
	static std::mt19937_64 mt64(0);
	std::uniform_int_distribution<uint64_t> get_rand_uni_int(0, 100);
	std::normal_distribution<double> get_normal(
		static_cast<double>(size)/2, static_cast<double>(size)/6);


	//std::random_device rd;
	//std::mt19937 gen(rd());

	// 平均0、標準偏差1の正規分布を定義
	//
	// ガウス分布に従う乱数を生成
	// for (int n = 0; n < 10; ++n) {
	//     std::cout << d(gen) << std::endl;
	// }

	for (int i = 0 ; i < NUM ; i++) {
		for (int j = 0 ; j < (100 * size) ; j++) {
			double val = get_normal(mt64);
			gH1hist[i]->Fill(val);
			int valx = get_normal(mt64);
			int valy = get_normal(mt64);
			gH2hist[i]->Fill(valx, valy);
		}
	}

	return;
}




int main(int argc, char* argv[])
{
	for (int i = 1 ; i < argc ; i++) {
		std::string ss(argv[i]);
		if (ss == "--dump") {
		} else
		if (ss[0] != '-') {
		}
	}

	std::string fullname(argv[0]);
	std::string filename = fullname.substr(fullname.rfind("/") + 1);

	auto start = std::chrono::high_resolution_clock::now();
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

	//std::vector<int> sizes = {10, 20, 40, 60, 80, 100, 200, 400, 600, 1000};
	std::vector<int> sizes = {10, 20, 40, 60, 80, 100, 1000};
	for (const auto &j : sizes) {

		int size = j;
		gHistInit(j, const_cast<char *>(filename.c_str()));
		gHistBook(j);

		start = std::chrono::high_resolution_clock::now();
		for (int i = 0 ; i < 10 ; i++) {
			gHist1Draw();
		}
		end = std::chrono::high_resolution_clock::now();
		duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		std::cout << "Size: " << std::setw(4) << size
			<< " 1D elapse: " << std::setw(6)
			<< (duration.count() / 10 / NUM) << " us";
	
		start = std::chrono::high_resolution_clock::now();
		for (int i = 0 ; i < 10 ; i++) {
			gHist2Draw();
		}
		end = std::chrono::high_resolution_clock::now();
		duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		// std::cout << "2D Size: " << std::setw(4) << size
		std::cout << " 2D elapse: " << std::setw(6)
			<< (duration.count() / 10 / NUM) << " us" << std::endl;
	}


	return 0;
}

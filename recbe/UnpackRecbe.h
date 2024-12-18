/*
 *
 */

#ifndef INC_RECBE
#define INC_RECBE

#include <iostream>
#include <vector>

#include <arpa/inet.h>

namespace Recbe {
inline namespace v0 {

#if 0
#define T_RAW		0x01
#define T_SUPPRESS	0x02
#define T_BOTH		0x03
#define T_RAW_OLD	0x22
#define T_SUPPRESS_OLD	0x20
#else
constexpr int T_RAW	     = 0x01;
constexpr int T_SUPPRESS     = 0x02;
constexpr int T_BOTH         = 0x03;
constexpr int T_RAW_OLD	     = 0x22;
constexpr int T_SUPPRESS_OLD = 0x20;
#endif

constexpr int N_CH = 48;

// network byte order
#pragma pack(1)
struct Header {
	unsigned char  Type;
	unsigned char  Id;
	unsigned short SentNumber;
	unsigned short TimeStamp;
	unsigned short Length;
	unsigned int   TriggerCount;
};

struct ChannelData {
	unsigned char ChannelId;
	unsigned char Length;
	unsigned short CountOverThreshold;
	unsigned short AdcSum;
	unsigned short TdcHit[2];
};


//Register map
constexpr unsigned int R_VERSION        = 0x04;
constexpr unsigned int R_MODE           = 0x05; // 0x01: RAW, 0x02: PROC, 0x03: RAW and PROC
constexpr unsigned int R_WINDOW_SIZE    = 0x06;
constexpr unsigned int R_DELAY          = 0x07;
constexpr unsigned int R_ASUM_TH0       = 0x08;
constexpr unsigned int R_ASUM_TH1       = 0x09;
constexpr unsigned int R_ENA_MANCHESTER = 0x09;
constexpr unsigned int R_ENA_TOT        = 0x09;

constexpr unsigned int R_MODE_RAW       = 0x01;
constexpr unsigned int R_MODE_PROC      = 0x02;
constexpr unsigned int R_MODE_RAW_PROC  = 0x03;

struct Tdc {
	unsigned int Value;
	bool Hit;
};

struct Data {
	std::vector< std::vector<unsigned int> > Adc;
	std::vector< std::vector<struct Tdc> > Tdc;
	std::vector< struct ChannelData > HitChannel;
	int nSample;
	int Type;
	int Id;
	int SentNumber;
	int TimeStamp;
	int Length;
	unsigned int TriggerCount;
};


int UnpackRaw(char *raw, struct Data &data)
{
	struct Header *header = reinterpret_cast<struct Header *>(raw);

	data.Type = header->Type;
	data.Id = header->Id;
	data.SentNumber = ntohs(header->SentNumber);
	data.TimeStamp = ntohs(header->TimeStamp);
	data.Length = ntohs(header->Length);
	data.TriggerCount = ntohl(header->TriggerCount);

	int nsample = data.Length / 2 / 2 / N_CH; // Length does not contain Header.
	data.nSample = nsample;

	data.Adc.clear();
	data.Adc.resize(N_CH);
	data.Tdc.clear();
	data.Tdc.resize(N_CH);

	for (int i = 0 ; i < nsample ; i++) {
		uint16_t * pAdc = reinterpret_cast<uint16_t *>(raw
			+ sizeof(struct Header) + (N_CH * sizeof(uint16_t) * (i * 2)));
		uint16_t * pTdc = reinterpret_cast<uint16_t *>(raw
			+ sizeof(struct Header) + (N_CH * sizeof(uint16_t) * (i * 2 + 1)));
		for (int j = 0 ; j < N_CH ; j++) {
			data.Adc[j].emplace_back(ntohs(*(pAdc++)));
			Tdc tdc;
			unsigned short tval = ntohs(*pTdc);
			tdc.Value = tval & 0x7fff;
			tdc.Hit = (tval & 0x8000) == 0x8000;
			data.Tdc[j].emplace_back(tdc);
			pTdc++;
		}
	}

	return 0;
}

int UnpackSupp(char *raw, struct Data &data)
{
	std::cout << "#W This part have not tested yet!" << std::endl;

	struct Header *header = reinterpret_cast<struct Header *>(raw);

	data.Type = header->Type;
	data.Id = header->Id;
	data.SentNumber = ntohs(header->SentNumber);
	data.TimeStamp = ntohs(header->TimeStamp);
	data.Length = ntohs(header->Length);
	data.TriggerCount = ntohl(header->TriggerCount);

	data.nSample = 0;
	data.Adc.clear();
	data.Adc.resize(0);
	data.Tdc.clear();
	data.Tdc.resize(0);

	data.HitChannel.clear();
	data.HitChannel.resize(0);

	char *pdata = raw + sizeof(struct Header);
	while (pdata - raw < data.Length) {
		struct ChannelData hit;
		memcpy(pdata, &hit, sizeof(struct ChannelData));
		data.HitChannel.emplace_back(hit);
		pdata += sizeof(struct ChannelData);
	}

	return 0;
}

int Unpack(char *raw, struct Data &data)
{
	struct Header *header = reinterpret_cast<struct Header *>(raw);
	if (header->Type == T_RAW) return UnpackRaw(raw, data);
	if (header->Type == T_RAW_OLD) return UnpackRaw(raw, data);
	if (header->Type == T_SUPPRESS) return UnpackSupp(raw, data);
	if (header->Type == T_SUPPRESS_OLD) return UnpackSupp(raw, data);
	return -1;
}

} // namespace v0
} // namespace Recbe
#endif

#if 0
int main(int argc, char* argv[])
{
	Recbe::Data data;

	return 0;
}
#endif

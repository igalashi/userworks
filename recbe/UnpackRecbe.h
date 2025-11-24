/*
 *
 */

#ifndef INC_RECBE
#define INC_RECBE

#include <iostream>
#include <vector>

#include <cstring>
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
	//std::cout << "#W This part have not tested yet!" << std::endl;

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

#if 0
	std::cout << "#D " << std::hex
		<< " " << static_cast<int>(raw[0] & 0xff)
		<< " " << static_cast<int>(raw[1] & 0xff)
		<< " " << static_cast<int>(raw[2] & 0xff)
		<< " " << static_cast<int>(raw[3] & 0xff)
		<< " " << static_cast<int>(raw[4] & 0xff)
		<< " " << static_cast<int>(raw[5] & 0xff)
		<< " " << static_cast<int>(raw[6] & 0xff)
		<< " " << static_cast<int>(raw[7] & 0xff)
		<< " " << static_cast<int>(raw[8] & 0xff)
		<< " " << static_cast<int>(raw[9] & 0xff)
		<< " " << static_cast<int>(raw[10] & 0xff)
		<< " " << static_cast<int>(raw[11] & 0xff)
		<< " :"
	       	<< " " << static_cast<int>(raw[12] & 0xff)
		<< " " << static_cast<int>(raw[13] & 0xff)
		<< " " << static_cast<int>(raw[14] & 0xff)
		<< " " << static_cast<int>(raw[15] & 0xff)
		<< " " << static_cast<int>(raw[16] & 0xff)
		<< " " << static_cast<int>(raw[17] & 0xff)
		<< std::endl;
#endif

#if 0
	std::cout << "#D" << std::dec
		<< " Sent: " << data.SentNumber << " : " << ntohs(header->SentNumber)
		<< " Len: " << data.Length << " : " << ntohs(header->Length)
		<< std::endl;
#endif

	char *pdata = raw + sizeof(struct Header);
	while ((pdata - raw + 8) <=
		static_cast<long int>((data.Length + sizeof(struct Header)))) {
		struct ChannelData *phit = reinterpret_cast<struct ChannelData *>(pdata);
		struct ChannelData hit;
		hit.ChannelId = phit->ChannelId;
		hit.Length = phit->Length;
		hit.CountOverThreshold = ntohs(phit->CountOverThreshold);
		hit.AdcSum = ntohs(phit->AdcSum);
		hit.TdcHit[0] = ntohs(phit->TdcHit[0]);
		if (hit.Length == 8) {
			hit.TdcHit[1] = 0x0000;
		} else if (hit.Length == 10) {
			hit.TdcHit[1] = ntohs(phit->TdcHit[1]);
		} else {
			std::cout << "#W irrgail Channel Data length : " << static_cast<int>(hit.Length & 0xff) << std::endl;
		}
		data.HitChannel.emplace_back(hit);

#if 0
		//std::cout << "#D " << static_cast<int>(hit.ChannelId) << std::endl;
		std::cout << "#D " << std::hex
			<< " " << reinterpret_cast<unsigned long long>(pdata)
			<< " ;" << std::setw(3) << std::dec << (pdata - raw) << " :"
			<< std::hex
			<< " " << std::setw(2) << static_cast<int>(pdata[0] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[1] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[2] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[3] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[4] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[5] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[6] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[7] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[8] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[9] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[10] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[11] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[12] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[13] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[14] & 0xff)
			<< " " << std::setw(2) << static_cast<int>(pdata[15] & 0xff)
			<< std::endl;
#endif

		//pdata += sizeof(struct ChannelData);
		pdata += hit.Length;
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
	struct Recbe::Data data;
	struct Recbe::Tdc tdc;

	return 0;
}
#endif

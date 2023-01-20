/*
 *
 */
#include <iostream>
#include <vector>
#include <random>

#define FULL 1048576

int time_hit_(std::vector<uint32_t> &hit, int nmean)
{
	std::mt19937 mt;
	mt.seed(12345);

	std::poisson_distribution<> prand(static_cast<double>(nmean));
	int n = prand(mt);

	std::uniform_int_distribution<> irand(0, FULL - 1);
	for (int i = 0 ; i < n ; i++) {
		uint32_t val = static_cast<uint32_t>(irand(mt));
		hit.push_back(val);
	}

	return 0;
}

std::mt19937 mt;
int time_hit(std::vector<uint32_t> &hit, int nmean)
{
	static bool at_first = true;
	if (at_first) {
		mt.seed(12345);
		at_first = false;
	}

	int val = 0;
	double meanstep = static_cast<double>(nmean) / static_cast<double>(FULL);
	std::exponential_distribution<> erand(static_cast<double>(meanstep));

	while (val < FULL) {
		int inc = static_cast<int>(erand(mt));
		if (inc > 10) {
			val += inc;
			if (val >= FULL) break;
			hit.push_back(val);
		}
	}


	return 0;
}

int time_mix(
	std::vector<uint32_t> &in1,
	std::vector<uint32_t> &in2,
	std::vector<uint32_t> &out)
{
	std::vector<uint32_t>::iterator it1 = in1.begin();
	std::vector<uint32_t>::iterator it2 = in2.begin();

	while ((it1 != in1.end()) || (it2 != in2.end())) {

		if ((it1 != in1.end()) && (it2 == in2.end())) {
			out.push_back(*(it1++));
		} else 
		if ((it1 == in1.end()) && (it2 != in2.end())) {
			out.push_back(*(it2++));
		} else
		if (*it1 < *it2) {
			out.push_back(*(it1++));
		} else {
			out.push_back(*(it2++));
		}
	}

	return 0;
}

int make_dummy_dataset(std::vector< std::vector<uint32_t> > &data, int nch)
{
	std::vector<uint32_t> signal;
	time_hit(signal, 1000);
	data.push_back(signal);

	for (int i = 1 ; i < nch ; i++) {
		std::vector<uint32_t> noise;
		time_hit(noise, 2000);

		std::vector<uint32_t> hit;
		time_mix(signal, noise, hit);

		data.push_back(hit);
	}
	
	return 0;
}

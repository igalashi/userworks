#pragma once

#include <vector>
#include <random>

class GaussianGenerator {
public:
    GaussianGenerator(double mean = 0.0, double stddev = 1.0, unsigned int seed = 0);
    
    std::vector<double> generateHistogramData(int numBins, int numSamples, double minX, double maxX);
    
    void setParameters(double mean, double stddev);
    void setSeed(unsigned int seed);

private:
    std::mt19937 m_generator;
    std::normal_distribution<double> m_distribution;
    double m_mean;
    double m_stddev;
};

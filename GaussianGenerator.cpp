#include "GaussianGenerator.h"
#include <algorithm>

GaussianGenerator::GaussianGenerator(double mean, double stddev, unsigned int seed)
    : m_generator(seed), m_distribution(mean, stddev), m_mean(mean), m_stddev(stddev) {
}

void GaussianGenerator::setParameters(double mean, double stddev) {
    m_mean = mean;
    m_stddev = stddev;
    m_distribution = std::normal_distribution<double>(mean, stddev);
}

void GaussianGenerator::setSeed(unsigned int seed) {
    m_generator.seed(seed);
}

std::vector<double> GaussianGenerator::generateHistogramData(int numBins, int numSamples, double minX, double maxX) {
    std::vector<double> binCounts(numBins, 0.0);
    double binWidth = (maxX - minX) / numBins;
    
    for (int i = 0; i < numSamples; ++i) {
        double value = m_distribution(m_generator);
        
        if (value >= minX && value < maxX) {
            int binIndex = static_cast<int>((value - minX) / binWidth);
            if (binIndex >= 0 && binIndex < numBins) {
                binCounts[binIndex]++;
            }
        }
    }
    
    return binCounts;
}

#ifndef Examples_Sampler_h
#define Exapmles_Sampler_h

#include <string>

#include <FairMQDevice.h>

class Sampler : public FairMQDevice
{
  public: 
  Sampler();
  ~Sampler() override = default;

  protected:
    std::string fText;
    uint64_t fMaxIterations;
    uint64_t fNumIterations;

    uint32_t fId;

    void Init() override;
    void InitTask() override;
    bool ConditionalRun() override;
    void PostRun() override;
    void PreRun() override;
    void Run() override;

};

#endif

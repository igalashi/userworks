#ifndef Examples_NullDevice_h
#define Examples_NullDevice_h

#include <FairMQDevice.h>

class NullDevice : public FairMQDevice
{
public: 
  NullDevice() = default;
  ~NullDevice() override = default;

protected:
  void Bind() override;
  bool ConditionalRun() override;
  void Connect() override;
  void Init() override;
  void InitTask() override;
  void PostRun() override;
  void PreRun() override;
  void Reset() override;
  void ResetTask() override;
  void Run() override; 

};

#endif
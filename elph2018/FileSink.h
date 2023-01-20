#ifndef FileSink_h
#define FileSink_h

#include <memory>
#include <string>
#include <string_view>

#include <FairMQDevice.h>

namespace highp::e50 {

class FileSink : public FairMQDevice
{
public: 
  struct OptionKey {
    static constexpr std::string_view RunId            {"run-id"};
    static constexpr std::string_view BufferSize       {"buffer-size"};
    static constexpr std::string_view FileType         {"file-type"};
    static constexpr std::string_view FilePath         {"file-path"};
    static constexpr std::string_view SubId            {"sub-id"};
    static constexpr std::string_view Overwrite        {"overwrite"};
    static constexpr std::string_view InputChannelName {"in-chan-name"};
  };

  struct OverwriteOption {
    static constexpr std::string_view None    {"none"};
    static constexpr std::string_view Warning {"warning"};
    static constexpr std::string_view Error   {"error"};
    static constexpr std::string_view Auto    {"auto"};
  };

  struct Impl;

  FileSink();
  FileSink(const FileSink&)            = delete;
  FileSink& operator=(const FileSink&) = delete;
  ~FileSink() = default; 

protected:
  void Init() override;
  void InitTask() override;
  bool HandleData(FairMQParts& msgParts, int index);
  void PreRun() override;
  void PostRun() override;

private:
  int fRunId {-1};
  int fBufferSize {1};
  std::string fOverwrite;
  std::string fFilePath;
  std::string fFileType;
  std::string fInputChannelName;
  std::string fSubId;
  std::unique_ptr<Impl> fImpl;

};

} // namespace highp::e50



#endif

#include <algorithm>
#include <cassert>
#include <fstream>
#include <unordered_map>
#include <map>
#include <stdexcept>
#include <functional>

// Alternative to printf
#include <boost/format.hpp>

#include <TTree.h>
#include <TFile.h>

#include "utility/filesystem.h"
#include "utility/Compressor.h"
#include "MessageUtil.h"
#include "Reporter.h"

#include "FileSink.h"

namespace Compressor = highp::e50::Compressor;

static const std::map<std::string_view, highp::e50::Compressor::Format> fileTypes =
{
  {".dat",  Compressor::Format::none},
  {".gz",   Compressor::Format::gzip},
  {".bz2",  Compressor::Format::bzip2},
  {".xz",   Compressor::Format::lzma},
  {".zstd", Compressor::Format::zstd},
  {".root", Compressor::Format::none}, 
};

using CompressFunc = std::function<std::vector<char>(const std::vector<char>&)>;
CompressFunc BufCompress;

//______________________________________________________________________________
struct highp::e50::FileSink::Impl
{
  Impl(){}
  ~Impl();

  void Write(FairMQParts& parts); 

  stdfs::path pathname;
  std::ofstream fStream;
  std::vector<char> fBuffer;
  std::string fFileType;

  std::unique_ptr<TFile> fRootFile;
  TTree* fTree {nullptr};
  std::vector<std::vector<char>> fBranchBuffer;
};

//______________________________________________________________________________
highp::e50::
FileSink::Impl::~Impl()
{
  LOG(info) << "close file : " << pathname;
  if (fStream.is_open()) {
    fStream.flush();
    fStream.close();
  }
  if (fRootFile) {
    fRootFile->Write();
    fRootFile->Close();
  }

  stdfs::permissions(pathname, stdfs::perms::owner_read | stdfs::perms::group_read | stdfs::perms::others_read);
}

//______________________________________________________________________________
void
highp::e50::
FileSink::Impl::Write(FairMQParts& parts)
{
  if (fTree) {
    fBranchBuffer.clear();
    fBranchBuffer.reserve(parts.Size());
    for (auto& m : parts) {
      fBranchBuffer.emplace_back(reinterpret_cast<char*>(m->GetData()), 
                                 reinterpret_cast<char*>(m->GetData()) + m->GetSize());
    }
    fTree->Fill();
  }

  if (fStream.is_open()) {
    // for (auto& m : parts) {
    //   auto bufSize = static_cast<int>(m->GetSize()*1.2); // <-- not optimized ...
    //   auto buf     = BufCompress(reinterpret_cast<char*>(m->GetData()), m->GetSize());
    //   fStream.write(buf.data(), buf.size());
    // }

    // merge messages into 1 vector before compression and file write
    auto len = MessageUtil::TotalLength(parts);
    std::vector<char> rawBuf;
    rawBuf.reserve(len);
    for (auto& m : parts) {
      rawBuf.insert(rawBuf.end(), 
                    std::make_move_iterator(reinterpret_cast<char*>(m->GetData())), 
                    std::make_move_iterator(reinterpret_cast<char*>(m->GetData()+m->GetSize())));
    }
    if (fFileType == ".dat") {
      fStream.write(rawBuf.data(), len);
    }
    else {
      auto bufSize = static_cast<int>(len*1.2); // <-- not optimized ...
      auto buf     = BufCompress(rawBuf);
      fStream.write(buf.data(), buf.size());
    }
  }
      
}

//______________________________________________________________________________
highp::e50::
FileSink::FileSink()
  : FairMQDevice()
{
}

//______________________________________________________________________________
void
highp::e50::
FileSink::Init()
{
  Reporter::Instance(fConfig);
}

//______________________________________________________________________________
void
highp::e50::
FileSink::InitTask()
{
  using opt = OptionKey;

  fRunId             = fConfig->GetValue<int>(opt::RunId.data());

  fBufferSize        = fConfig->GetValue<int>(opt::BufferSize.data());
  fFilePath          = fConfig->GetValue<std::string>(opt::FilePath.data());
  fFileType          = fConfig->GetValue<std::string>(opt::FileType.data());
  fSubId             = fConfig->GetValue<std::string>(opt::SubId.data());

  fOverwrite         = fConfig->GetValue<std::string>(opt::Overwrite.data());
  fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());

  fBufferSize *= 1024 * 1024;

  OnData(fInputChannelName, &FileSink::HandleData);
  
  Reporter::Reset();
}

//______________________________________________________________________________
// handler is called whenever a message arrives on "data", with a reference to the message and a sub-channel index (here 0)
bool
highp::e50::
FileSink::HandleData(FairMQParts& msgParts, int index)
{
  assert(msgParts.Size()!=0);
  assert(msgParts.Size()>=2);

  Reporter::AddInputMessageSize(msgParts);

  fImpl->Write(msgParts);
  return true;
}


//______________________________________________________________________________
void
highp::e50::
FileSink::PreRun()
{
  // check file type
  if (fileTypes.find(fFileType) == fileTypes.end()) {
    LOG(ERROR) << "Unknown file type. " << fFileType;
    throw std::runtime_error("unknown file type : " + fFileType);
  }


  std::string extension;
  if (fFileType!=".root" && fFileType.find(".dat")==std::string::npos) {
    if (fileTypes.find(fFileType)!=fileTypes.end()) extension = ".dat" + fFileType;
  }
  if (extension.empty()) extension = fFileType;


  {
    auto format = fileTypes.at(fFileType);
    //BufCompress = [format, bufSize=fBufferSize](const char* src, int n) -> std::vector<char> {
    //return std::move(Compressor::Compress<char>(src, n, format, n, bufSize));
    //};
    BufCompress = [format, bufSize=fBufferSize](const auto& v) -> std::vector<char> {
      return std::move(Compressor::Compress(v, format, bufSize, bufSize));
    };
    // LOG(debug) << "Compression format type = " << static_cast<int>(format) << std::endl;
  }

  if (!fSubId.empty()) fSubId = "_" + fSubId;


  std::string filename { (boost::format("%s/run%06d%s%s") % fFilePath % fRunId % fSubId % extension).str() };
  stdfs::path pathName(filename);
  while (stdfs::exists(pathName)) {
    if (fOverwrite == OverwriteOption::Auto) {
      LOG(WARNING) << "File name : " << filename << " already exists. auto increment run-id : "
                   << fRunId << " -> " << (++fRunId);
      filename = (boost::format("%s/run%06d%s%s") % fFilePath % fRunId % fSubId % extension).str();
      pathName = filename;
      continue;
    }
    if (fOverwrite == OverwriteOption::Warning) {
      LOG(WARNING) << "File name : " << filename << " already exists. overwrite";
    } else if (fOverwrite == OverwriteOption::Error) {
      LOG(ERROR) <<  "File name : " << filename << " already exists. stop";
      throw std::runtime_error("file already exists " + filename);
    } else {
      LOG(ERROR) << "Unknown overwrite option : " << fOverwrite; 
      throw std::runtime_error("unknown overwrite option : " + fOverwrite);
    }
    break;
  }
  LOG(info) << "File name : " << filename;
                                                                                  
  fImpl = std::make_unique<Impl>();

  if (fFileType==".root") {
    fImpl->fRootFile = std::make_unique<TFile>(filename.data(), "recreate");
    fImpl->fTree = new TTree("tree", "tree");
    fImpl->fTree->Branch("data", &(fImpl->fBranchBuffer));
    return;
  }
  fImpl->fFileType = fFileType;

  fImpl->fBuffer.resize(fBufferSize);
  // call pubsetbuf to change buffer size before file open
  fImpl->fStream.rdbuf()->pubsetbuf(fImpl->fBuffer.data(), fBufferSize);
  fImpl->fStream.open(filename.data(), std::ios::binary);

  LOG(warn) << " run : " << fRunId << ".  open file : " << filename;
  fImpl->pathname = pathName;
}

//______________________________________________________________________________
void
highp::e50::
FileSink::PostRun()
{
  LOG(warn) << " run : " << fRunId << " stopped. size = " << stdfs::file_size(fImpl->pathname) << " bytes";

  // destroy unique_ptr
  fImpl.reset(nullptr);
 
}


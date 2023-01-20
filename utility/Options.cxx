#include <functional>
#include <map>
#include <sstream>

#include <yaml-cpp/yaml.h>

#include "utility/Options.h"
#include "utility/filesystem.h"

using namespace e16::daq::debug;

namespace {
const std::string MyFile(e16::filename(__FILE__));
const std::string MyClass(e16::stem(__FILE__));

// operator overload for custom ostream, which is returned from a function created by lambda
template <typename T>
std::ostream &operator<<(std::function<std::ostream &(void)> os, const T &v)
{
   return os() << v;
}

// overload for std::endl (std::endl is function template)
using ostream_manipulator = std::ostream &(*)(std::ostream &);
std::ostream &operator<<(std::function<std::ostream &(void)> os, ostream_manipulator manip)
{
   manip(os());
   return os();
}

//______________________________________________________________________________
// private class
//______________________________________________________________________________

struct Parser {
   Parser(e16::Options &opt, const e16::DebugFlags &debug) : fTable(opt.Get()), fDebug(debug) {}
   ~Parser() = default;

   bool IsString(const YAML::Node &node) const;
   void Parse(const YAML::Node &node);

   e16::Options::Map_t &fTable;
   e16::DebugFlags fDebug;
   std::string fId{};
   std::string fApp{};
   std::vector<std::string> fKeys{};
};

//______________________________________________________________________________
bool Parser::IsString(const YAML::Node &node) const
{
   try {
      node.as<std::string>();
   } catch (const YAML::BadConversion &e) {
      std::cerr << "#Exception: " << MyFile << ": " << __LINE__ << " " << __FUNCTION__ << e.what() << std::endl;
      return false;
   }
   return true;
}

//______________________________________________________________________________
void Parser::Parse(const YAML::Node &node)
{
   std::ostringstream dummy_out;
   auto cout = [&dummy_out, this]() -> std::ostream & { return (fDebug[Flag::OptionsParse]) ? std::cout : dummy_out; };

   if (!fKeys.empty()) {
      cout << " keys.size = " << fKeys.size() << " keys.back = " << fKeys.back() << std::endl;
   }

   if (node.IsMap()) {
      cout << node.Tag() << " " << node.Scalar() << " map. size = " << node.size() << std::endl;
      for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
         auto key = it->first.as<std::string>();
         fKeys.push_back(key);
         if (!fId.empty() && (fKeys.size() == 1)) {
            fApp = key;
            fTable[fId][fApp];
            cout << " add app " << fApp << std::endl;
         }
         Parse(it->second);
      }
   } else if (node.IsSequence()) {
      cout << node.Tag() << " " << node.Scalar() << " sequence. size = " << node.size() << std::endl;
      cout << fKeys.back() << ": " << std::endl;
      auto &vm = fTable[fId][fApp];
      std::vector<std::string> v;
      for (const auto &n : node) {
         v.push_back(n.as<std::string>());
         cout << n.as<std::string>() << " ";
      }
      cout << std::endl;
      vm.insert({fKeys.back(), v});

   } else if (node.IsScalar()) {
      cout << fKeys.back() << ": " << node.Tag() << " " << node.Scalar() << " scalar. size = " << node.size()
           << std::endl;
      if (fKeys.back() == "id" && IsString(node)) {
         const auto v = node.as<std::string>();
         cout << " add ID: " << v << std::endl;
         fId = v;
         fTable[v];
      } else {
         auto &vm = fTable[fId][fApp];
         vm.insert({fKeys.back(), node.as<std::string>()});
      }
   } else if (node.IsNull()) {
      std::cout << " node is null " << std::endl;
   } else if (!node.IsDefined()) {
      std::cout << " node is undefined " << std::endl;
   }

   cout << MyFile << ":" << __LINE__ << " ";
   if (!fKeys.empty()) {
      cout << fKeys.back();
      fKeys.pop_back();
   }
   cout << std::endl;
}

} // namespace

//______________________________________________________________________________
void e16::Options::AppRecord::Print() const
{
   std::cout << "  " << fName << ": \n";

   // sort by name
   std::map<std::string, std::any> l(fList.begin(), fList.end());

   for (const auto &[k, v] : l) {
      std::cout << "    " << k << " : ";
      if (v.has_value()) {
         auto &t = v.type();
         if (t == typeid(std::string)) {
            const auto &s = std::any_cast<std::string>(v);
            std::cout << " " << s;
         } else if (t == typeid(std::vector<std::string>)) {
            const auto &s = std::any_cast<std::vector<std::string>>(v);
            std::cout << " (vector) ";
            for (const auto &e : s) {
               std::cout << " " << e;
            }
         }
         std::cout << "\n";
      } else {
         std::cout << " (has_value = false)\n";
      }
   }
   std::cout << std::flush;
}

// ---------- private class end ----------------------------------------

//______________________________________________________________________________
e16::Options::Options(std::size_t debug) : fDebug()
{
   if (debug < fDebug.size()) {
      fDebug.set(static_cast<Flag>(debug));
   }
}

//______________________________________________________________________________
e16::Options::Options(const e16::DebugFlags &debug) : fDebug(debug) {}

//______________________________________________________________________________
const e16::Options::AppRecord e16::Options::GetAppRecord(std::string_view id, std::string_view app) const
{
   const auto &mVar = fTable.at(id.data()).at(app.data());
   return {app.data(), mVar};
}

//______________________________________________________________________________
void e16::Options::Parse(std::string_view file)
{
   Parser p(*this, fDebug);
   const auto &top = YAML::LoadFile(file.data());
   for (const auto &node : top) {
      // std::cout << MyFile << ":" << __LINE__
      //           << " --------------------------------------------------" << std::endl;
      p.Parse(node);
   }
}

//______________________________________________________________________________
void e16::Options::Parse(const std::vector<std::string> &files)
{
   for (std::string_view f : files) {
      Parse(f);
   }
}

//______________________________________________________________________________
void e16::Options::Print() const
{
   if (fTable.empty()) {
      std::cout << "Options::Print() not initialized" << std::endl;
      return;
   }

   // sort by name
   std::map<std::string, std::map<std::string, AppRecord>> table;
   for (const auto &[id, ms] : fTable) {
      auto &apps = table[id];
      for (const auto &[appName, m] : ms) {
         for (const auto &[k, v] : m) {
            auto &rec = apps[appName];
            rec.fName = appName;
            rec.fList[k] = v;
         }
      }
   }

   for (const auto &[id, apps] : table) {
      std::cout << "--------------------\n id = " << id << "\n";
      for (const auto &[appName, app] : apps) {
         app.Print();
      }
   }
   std::cout << std::endl;
}

//______________________________________________________________________________
std::vector<std::pair<std::string, std::string>> e16::Options::ParsePairs(std::string_view arg)
{
   const auto &node = YAML::Load(arg.data());
   if (!node.IsSequence()) {
      std::cout << " node is not sequence" << std::endl;
      return {};
   }

   auto ParseScalar = [](auto &n, auto &p, auto &v) {
      if (p.first.empty())
         p.first = n.template as<std::string>();
      else if (p.second.empty()) {
         p.second = n.template as<std::string>();
         v.push_back(p);
         p.first.clear();
         p.second.clear();
      }
   };

   std::vector<std::pair<std::string, std::string>> v;
   std::pair<std::string, std::string> p;

   for (const auto &sub : node) {
      if (sub.IsScalar()) {
         ParseScalar(sub, p, v);
      } else if (sub.IsSequence()) {
         for (const auto &subsub : sub) {
            ParseScalar(subsub, p, v);
         }
      }
   }

   return v;
}

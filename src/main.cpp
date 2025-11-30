#include "eglyf.hpp"

#include <cxxopts.hpp>

static int Fail(eglyf::Status st) {
  st.print(std::cout, "");
  return -1;
}

static eglyf::Optional<std::u8string> U8StringFromString(std::string const &s) {
  using namespace std;
  u8string ret;
  for (char ch : s) {
    if (0x20 <= ch && ch <= 0x7e) {
      ret.push_back((char8_t)ch);
    } else {
      return EGLYF_NULLOPT;
    }
  }
  return ret;
}

int main(int argc, char *argv[]) {
  using namespace std;
  using namespace eglyf;

  cxxopts::Options options("eglyf", "An OpenType font tool that modifies OTF files to add support for Egyptian Hieroglyph Format Controls.");
  // clang-format off
  options.add_options()
    ("i,input", "Input font file path", cxxopts::value<string>())
    ("o,output", "Output font file path", cxxopts::value<string>())
    ("names", "Font names formatted as family/subFamily/fullName/psName (ex.: --names \"My EgyptHiero/Regular/My Egyptian Hieroglyphs Regular/MyEgyptianHieroglyphs-Regular\")", cxxopts::value<string>())
    ("experimental-mdc-subst", "Switch experimental GSUB lookups for MdC support (on/off, default off)", cxxopts::value<string>()->default_value("off"))
  ;
  // clang-format on

  auto result = options.parse(argc, argv);

  Config cfg;

  string input = result["input"].as<string>();
  string output = result["output"].as<string>();

  optional<string> names = result["names"].as_optional<string>();
  if (names && !names->empty()) {
    vector<u8string> tokens;
    size_t offset = 0;
    size_t pos = names->find('/', offset);
    while (pos != string::npos) {
      auto sub = names->substr(offset, pos - offset);
      auto sub8 = U8StringFromString(sub);
      if (!sub8) {
        return Fail(sub8.status());
      }
      tokens.push_back(*sub8);
      offset = pos + 1;
      pos = names->find('/', offset);
    }
    {
      auto sub = names->substr(offset);
      auto sub8 = U8StringFromString(sub);
      if (!sub8) {
        return Fail(sub8.status());
      }
      tokens.push_back(*sub8);
    }
    if (tokens.size() != 4) {
      cerr << "--name family/subFamily/fullName/psName" << endl;
      cerr << "ex: --name \"My EgyptHiero/Regular/My Egyptian Hieroglyphs Regular/MyEgyptianHieroglyphs-Regular\"" << endl;
      return -1;
    }
    Config::Name name;
    name.family = tokens[0];
    name.subFamily = tokens[1];
    name.fullName = tokens[2];
    name.psName = tokens[3];
    cfg.name = name;
  }

  auto mdc = result["experimental-mdc-subst"].as<string>();
  if (mdc != "on" && mdc != "off") {
    cerr << "invalid value for experimental-mdc-subst option: \"" << mdc << "\"" << endl;
    return -1;
  }
  cfg.enableSubstMdc = mdc == "on";

  FileInputStream fis(input);
  shared_ptr<Font> ff;
  if (auto st = Font::Read(fis, ff); !st.ok()) {
    return Fail(st);
  }
  if (auto st = Transformer::Transform(ff, cfg); !st.ok()) {
    return Fail(st);
  }
  FileOutputStream fos(output);
  if (auto st = ff->write(fos); !st.ok()) {
    return Fail(st);
  }
  return 0;
}

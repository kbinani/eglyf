#pragma once

namespace eglyf {

struct Config {
  struct Name {
    // ex: "My EgyptHiero"
    std::u8string family;
    // ex: "Regular"
    std::u8string subFamily;
    // ex: "My Egyptian Hieroglyphs Regular"
    std::u8string fullName;
    // ex: "MyEgyptianHieroglyphs-Regular"
    std::u8string psName;
  };

  std::optional<Name> name;
  bool enableSubstMdc = true;
};

} // namespace eglyf

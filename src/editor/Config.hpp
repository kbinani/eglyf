#pragma once

namespace eglyf {

struct Config {
  struct Name {
    // ex: "My EgyptHiero"
    std::u16string family;
    // ex: "Regular"
    std::u16string subFamily;
    // ex: "My Egyptian Hieroglyphs Regular"
    std::u16string fullName;
    // ex: "MyEgyptianHieroglyphs-Regular"
    std::u16string psName;
  };

  std::optional<Name> name;
  bool enableSubstMdc = true;
};

} // namespace eglyf

#pragma once

namespace eglyf {

class Insertion {
public:
  enum class Pos {
    TopStart,    // ts
    BottomStart, // bs
    TopEnd,      // te
    BottomEnd,   // be

    Top,    // ti
    Middle, // mi
    Bottom, // bi
  };

  static std::string StringFromPos(Pos p) {
    switch (p) {
    case Pos::TopStart:
      return "ts";
    case Pos::BottomStart:
      return "bs";
    case Pos::TopEnd:
      return "te";
    case Pos::BottomEnd:
      return "be";
    case Pos::Top:
      return "ti";
    case Pos::Middle:
      return "mi";
    case Pos::Bottom:
      return "bi";
    default:
      assert(false);
    }
  }

  struct Info {
    explicit Info(WxH size, std::optional<int> dx = std::nullopt, std::optional<int> dy = std::nullopt) : size(size), dx(dx), dy(dy) {}
    WxH size;
    std::optional<int> dx;
    std::optional<int> dy;
  };

  struct Plan {
    std::map<Pos, std::map<WxH, Info>> insertions;
  };

  static Status CreatePlan_(FontFile const &font, std::unordered_map<std::string, SizeVariants> const &sizeVariants, std::map<std::string, Plan> &out) {
    using namespace std;
    auto const ts = Pos::TopStart;
    auto const bs = Pos::BottomStart;
    auto const te = Pos::TopEnd;
    auto const be = Pos::BottomEnd;
    auto const ti = Pos::Top;
    auto const bi = Pos::Bottom;
    auto const mi = Pos::Middle;

    map<string, set<Pos>> positions;
    positions["A14"] = {bi};
    positions["A16"] = {te};
    positions["A18"] = {be};
    positions["A26"] = {bs};
    positions["A28"] = {bs, be};
    positions["A30"] = {bs};
    positions["A40"] = {ts};
    positions["A41"] = {ts};
    positions["A43"] = {ts};
    positions["A45"] = {ts};
    positions["A50"] = {ts};
    positions["A55"] = {bi};
    positions["B1"] = {ts};
    positions["C1"] = {ts};
    positions["C10"] = {ts};
    positions["C2a"] = {ts};
    positions["C9"] = {ts};
    positions["D17"] = {te};
    positions["D28"] = {ti, mi};
    positions["D28o"] = {mi, bi};
    positions["D3"] = {bs};
    positions["D32"] = {mi, bi};
    positions["D36"] = {ts, ti};
    positions["D40"] = {ti};
    positions["D41"] = {ts};
    positions["D42"] = {ts, ti};
    positions["D45"] = {ti};
    positions["D52"] = {bs};
    positions["D53"] = {bs};
    positions["D54"] = {ts};
    positions["D55"] = {te};
    positions["D56"] = {ts};
    positions["D58"] = {ts};
    positions["D60"] = {mi};
    positions["D66"] = {ts, ti};
    positions["E1"] = {te};
    positions["E10"] = {te};
    positions["E100"] = {te};
    positions["E11"] = {te};
    positions["E15"] = {bs, te, ti};
    positions["E16"] = {te, ti};
    positions["E16a"] = {te};
    positions["E17"] = {te};
    positions["E17a"] = {te};
    positions["E18"] = {bs, te};
    positions["E19"] = {te};
    positions["E20"] = {te, ti};
    positions["E20a"] = {te};
    positions["E21"] = {ti};
    positions["E22"] = {te};
    positions["E23"] = {te};
    positions["E27"] = {te};
    positions["E29"] = {te};
    positions["E3"] = {te};
    positions["E31"] = {te};
    positions["E32"] = {te};
    positions["E34"] = {te};
    positions["E38"] = {te};
    positions["E6"] = {te, bi};
    positions["E7"] = {te};
    positions["E8"] = {te};
    positions["E8a"] = {bs, te};
    positions["E9"] = {bs, te, ti};
    positions["F1"] = {ti};
    positions["F13"] = {ti};
    positions["F13a"] = {ti};
    positions["F16"] = {te};
    positions["F18"] = {te};
    positions["F19"] = {ts};
    positions["F20"] = {bs};
    positions["F29"] = {bs, be};
    positions["F30"] = {bs};
    positions["F39"] = {bs};
    positions["F4"] = {ts};
    positions["F40"] = {bi};
    positions["F45"] = {bs, be};
    positions["F45a"] = {bs, be};
    positions["F5"] = {ti};
    positions["F6"] = {te, ti};
    positions["G1"] = {bs};
    positions["G10"] = {te, be};
    positions["G11a"] = {bs, te};
    positions["G13"] = {te};
    positions["G14"] = {bs};
    positions["G15"] = {bs};
    positions["G17"] = {bs, te};
    positions["G18"] = {bs};
    positions["G2"] = {bs};
    positions["G20"] = {bs};
    positions["G21"] = {bs, te};
    positions["G22"] = {bs, te};
    positions["G23"] = {bs, te};
    positions["G25"] = {bs, te};
    positions["G26"] = {bs, te};
    positions["G26a"] = {bs};
    positions["G27"] = {bs, te};
    positions["G28"] = {bs, te};
    positions["G3"] = {bs};
    positions["G31"] = {bs, te};
    positions["G32"] = {te};
    positions["G33"] = {bs};
    positions["G34"] = {bs};
    positions["G35"] = {bs, te};
    positions["G36"] = {bs, te};
    positions["G37"] = {bs, te};
    positions["G38"] = {bs, te};
    positions["G39"] = {bs, te};
    positions["G4"] = {bs};
    positions["G41"] = {bs};
    positions["G42"] = {bs, te};
    positions["G43"] = {bs, be};
    positions["G44"] = {bs, te};
    positions["G45"] = {bs};
    positions["G47"] = {bs, te};
    positions["G5"] = {bs};
    positions["G50"] = {bs};
    positions["G53"] = {te};
    positions["G6"] = {bs};
    positions["G6a"] = {te};
    positions["G7"] = {bs, te};
    positions["G8"] = {te};
    positions["G9"] = {bs, te};
    positions["I1"] = {bs};
    positions["I10"] = {bs};
    positions["I10a"] = {bs, ti};
    positions["I11"] = {bs, te};
    positions["I3"] = {te};
    positions["I5"] = {te};
    positions["I7"] = {te};
    positions["I8"] = {bs, te};
    positions["I9"] = {te};
    positions["J13"] = {mi};
    positions["J15"] = {mi};
    positions["J19"] = {mi, bi};
    positions["J7"] = {ts, ti};
    positions["L1"] = {bi};
    positions["M10"] = {bs};
    positions["M26"] = {bs, be};
    positions["M27"] = {bs, be};
    positions["M9"] = {bs};
    positions["N11"] = {bi};
    positions["N2"] = {bs, be};
    positions["N3"] = {bs, be};
    positions["N36"] = {mi};
    positions["N37"] = {mi};
    positions["O1"] = {mi, bi};
    positions["O13a"] = {mi};
    positions["O14"] = {be};
    positions["O16"] = {mi};
    positions["O17"] = {mi};
    positions["O18"] = {mi};
    positions["O26"] = {mi};
    positions["O32"] = {mi, bi};
    positions["O36"] = {mi};
    positions["O6"] = {mi};
    positions["Q2"] = {ts};
    positions["R12"] = {bs, be};
    positions["R13"] = {te, be};
    positions["R8"] = {bs};
    positions["S1"] = {ts};
    positions["S2"] = {ts};
    positions["S22"] = {ti, bi};
    positions["S28"] = {bs};
    positions["T14"] = {bs};
    positions["T32"] = {bs};
    positions["T5"] = {bi};
    positions["T6"] = {bs};
    positions["T7a"] = {bs};
    positions["U1"] = {ts};
    positions["U15"] = {te};
    positions["U19"] = {te, ti};
    positions["U2"] = {ts, te};
    positions["U21"] = {te, ti, bi};
    positions["V10"] = {mi};
    positions["V10n"] = {mi};
    positions["V12"] = {bs};
    positions["V15"] = {bs};
    positions["V22"] = {bs};
    positions["V23"] = {bs};
    positions["V23a"] = {bs};
    positions["V6"] = {mi};
    positions["V7"] = {mi};
    positions["W4"] = {mi};
    positions["Z10"] = {ti, bi};
    positions["Z11"] = {bs, be};
    positions["Z6"] = {bs};

    return Status::Ok();
  }

  static Status CreatePlan(FontFile const &font, std::unordered_map<std::string, SizeVariants> const &sizeVariants, std::map<std::string, Plan> &out) {
    using namespace std;
    auto const ts = Pos::TopStart;
    auto const bs = Pos::BottomStart;
    auto const te = Pos::TopEnd;
    auto const be = Pos::BottomEnd;
    auto const ti = Pos::Top;
    auto const bi = Pos::Bottom;
    auto const mi = Pos::Middle;
    out["A14"] = Plan({
        {bi, {{66, Info(22)}}},
    });
    out["A16"] = Plan({
        {te, {{46, Info(21)}}},
    });
    out["A18"] = Plan({
        {be, {{46, Info(21)}}},
    });
    out["A26"] = Plan({
        {bs, {{46, Info(13)}}},
    });
    out["A28"] = Plan({
        {bs, {{56, Info(13)}}},
        {be, {{56, Info(13)}}},
    });
    out["A30"] = Plan({
        {bs, {{46, Info(14)}}},
    });
    out["A40"] = Plan({
        {ts, {{46, Info(13)}}},
    });
    out["A41"] = Plan({
        {ts, {{46, Info(13)}}},
    });
    out["A43"] = Plan({
        {ts, {{46, Info(13)}}},
    });
    out["A45"] = Plan({
        {ts, {{46, Info(13)}}},
    });
    out["A50"] = Plan({
        {ts, {{56, Info(23)}}},
    });
    out["A55"] = Plan({
        {bi, {{64, Info(42)}}},
    });
    out["B1"] = Plan({
        {ts, {{46, Info(13)}}},
    });
    out["C1"] = Plan({
        {ts, {{46, Info(13)}}},
    });
    out["C2a"] = Plan({
        {ts, {{36, Info(13)}}},
    });
    out["C9"] = Plan({
        {ts, {{36, Info(13)}}},
    });
    out["C10"] = Plan({
        {ts, {{36, Info(13)}}},
    });
    out["D3"] = Plan({
        {bs, {{63, Info(22)}}},
    });
    out["D17"] = Plan({
        {te, {{64, Info(32)}, {62, Info(31)}, {43, Info(21)}, {32, Info(11)}}},
    });
    out["D28"] = Plan({
        {mi, {{55, Info(23)}}},
        {ti, {{55, Info(24)}}},
    });
    out["D28o"] = Plan({
        {mi, {{55, Info(23)}}},
        {bi, {{55, Info(24)}}},
    });
    out["D32"] = Plan({
        {mi, {{56, Info(22)}}},
        {bi, {{56, Info(24)}}},
    });
    out["D36"] = Plan({
        {ts, {{63, Info(51)}}},
        {ti, {{66, Info(43)}}},
    });
    out["D40"] = Plan({
        {ti, {{63, Info(41)}}},
    });
    out["D41"] = Plan({
        {ts, {{66, Info(41)}}},
    });
    out["D42"] = Plan({
        {ts, {{63, Info(51)}}},
        {ti, {{63, Info(41)}}},
    });
    out["D45"] = Plan({
        {ti, {{64, Info(21)}}},
    });
    out["D52"] = Plan({
        {bs, {{63, Info(41)}}},
    });
    out["D53"] = Plan({
        {bs, {{63, Info(41)}}},
    });
    out["D54"] = Plan({
        {ts, {{54, Info(12)}}},
    });
    out["D55"] = Plan({
        {te, {{54, Info(12)}}},
    });
    out["D56"] = Plan({
        {ts, {{36, Info(15)}}},
    });
    out["D58"] = Plan({
        {ts, {{46, Info(24)}}},
    });
    out["D60"] = Plan({
        {mi, {{46, Info(13)}}},
    });
    out["D66"] = Plan({
        {ts, {{64, Info(42)}}},
        {ti, {{64, Info(22)}}},
    });
    out["E1"] = Plan({
        {te, {{66, Info(31)}}},
    });
    out["E3"] = Plan({
        {te, {{66, Info(41)}}},
    });
    out["E6"] = Plan({
        {te, {{66, Info(32)}}},
        {bi, {{66, Info(12)}}},
    });
    out["E7"] = Plan({
        {te, {{66, Info(32)}}},
    });
    out["E8"] = Plan({
        {te, {{66, Info(41)}}},
    });
    out["E8a"] = Plan({
        {bs, {{66, Info(32)}}},
        {te, {{66, Info(22)}}},
    });
    out["E9"] = Plan({
        {bs, {{65, Info(51)}}},
        {te, {{65, Info(31)}}},
        {ti, {{65, Info(21)}}},
    });
    out["E10"] = Plan({
        {te, {{65, Info(21)}}},
    });
    out["E11"] = Plan({
        {te, {{65, Info(21)}}},
    });
    out["E15"] = Plan({
        {bs, {{66, Info(52)}}},
        {te, {{66, Info(32)}}},
        {ti, {{66, Info(21)}}},
    });
    out["E16"] = Plan({
        {te, {{66, Info(32)}}},
        {ti, {{66, Info(21)}}},
    });
    out["E16a"] = Plan({
        {te, {{66, Info(22)}}},
    });
    out["E17"] = Plan({
        {te, {{66, Info(42)}}},
    });
    out["E17a"] = Plan({
        {te, {{66, Info(32)}}},
    });
    out["E18"] = Plan({
        {bs, {{66, Info(21)}}},
        {te, {{66, Info(41)}}},
    });
    out["E19"] = Plan({
        {te, {{66, Info(41)}}},
    });
    out["E20"] = Plan({
        {te, {{66, Info(32)}}},
        {ti, {{66, Info(23)}}},
    });
    out["E20a"] = Plan({
        {te, {{56, Info(21)}}},
    });
    out["E21"] = Plan({
        {ti, {{65, Info(22)}}},
    });
    out["E22"] = Plan({
        {te, {{64, Info(41)}}},
    });
    out["E23"] = Plan({
        {te, {{63, Info(31)}}},
    });
    out["E27"] = Plan({
        {te, {{46, Info(23)}}},
    });
    out["E29"] = Plan({
        {te, {{56, Info(32)}}},
    });
    out["E31"] = Plan({
        {te, {{66, Info(32)}}},
    });
    out["E32"] = Plan({
        {te, {{65, Info(31)}}},
    });
    out["E34"] = Plan({
        {te, {{64, Info(12)}}},
    });
    out["E38"] = Plan({
        {te, {{65, Info(41)}}},
    });
    out["E100"] = Plan({
        {te, {{64, Info(41)}}},
    });
    out["F1"] = Plan({
        {ti, {{44, Info(11)}}},
    });
    out["F4"] = Plan({
        {ts, {{66, Info(33)}, {65, Info(33)}, {64, Info(33)}, {63, Info(32)}, {62, Info(21)}, {54, Info(23)}, {52, Info(21)}, {32, Info(11)}}},
    });
    out["F5"] = Plan({
        {ti, {{56, Info(11)}}},
    });
    out["F6"] = Plan({
        {te, {{66, Info(12)}}},
        {ti, {{66, Info(11)}}},
    });
    out["F13"] = Plan({
        {ti, {{64, Info(24)}}},
    });
    out["F13a"] = Plan({
        {ti, {{64, Info(22)}}},
    });
    out["F16"] = Plan({
        {te, {{64, Info(31)}}},
    });
    out["F18"] = Plan({
        {te, {{62, Info(41)}}},
    });
    out["F19"] = Plan({
        {ts, {{63, Info(31)}}},
    });
    out["F20"] = Plan({
        {bs, {{66, Info(54)}, {56, Info(43)}, {63, Info(42)}, {33, Info(21)}}},
    });
    out["F29"] = Plan({
        {bs, {{56, Info(22)}}},
        {be, {{56, Info(22)}}},
    });
    out["F30"] = Plan({
        {bs, {{65, Info(42)}}},
    });
    out["F39"] = Plan({
        {bs, {{55, Info(31)}}},
    });
    out["F40"] = Plan({
        {bi, {{66, Info(42)}}},
    });
    out["F45"] = Plan({
        {bs, {{46, Info(14)}}},
        {be, {{46, Info(14)}}},
    });
    out["F45a"] = Plan({
        {bs, {{46, Info(14)}}},
        {be, {{46, Info(14)}}},
    });
    out["G1"] = Plan({
        {bs, {{66, Info(22, nullopt, 2)}}},
    });
    out["G2"] = Plan({
        {bs, {{66, Info(12, nullopt, 2)}}},
    });
    out["G3"] = Plan({
        {bs, {{66, Info(21, nullopt, 5)}}},
    });
    out["G4"] = Plan({
        {bs, {{66, Info(12, nullopt, 2)}}},
    });
    out["G5"] = Plan({
        {bs, {{66, Info(12, nullopt, 2)}}},
    });
    out["G6"] = Plan({
        {bs, {{66, Info(12, nullopt, 2)}}},
    });
    out["G6a"] = Plan({
        {te, {{66, Info(22)}}},
    });
    out["G7"] = Plan({
        {bs, {{56, Info(21)}}},
        {te, {{56, Info(21)}}},
    });
    out["G8"] = Plan({
        {te, {{56, Info(11)}}},
    });
    out["G9"] = Plan({
        {bs, {{56, Info(12)}}},
        {te, {{56, Info(32)}, {55, Info(22)}, {45, Info(22)}}},
    });
    out["G10"] = Plan({
        {te, {{66, Info(22)}}},
        {be, {{66, Info(11)}}},
    });
    out["G11a"] = Plan({
        {bs, {{66, Info(22)}}},
        {te, {{66, Info(12)}}},
    });
    out["G13"] = Plan({
        {te, {{66, Info(33)}}},
    });
    out["G14"] = Plan({
        {bs, {{66, Info(22, nullopt, 1)}}},
    });
    out["G15"] = Plan({
        {bs, {{66, Info(22, nullopt, 2)}}},
    });
    out["G17"] = Plan({
        {bs, {{66, Info(12, nullopt, 2)}}},
        {te, {{66, Info(23)}}},
    });
    out["G18"] = Plan({
        {bs, {{66, Info(12, nullopt, 2)}}},
    });
    out["G20"] = Plan({
        {bs, {{66, Info(22, nullopt, 2)}}},
    });
    out["G21"] = Plan({
        {bs, {{66, Info(21, nullopt, 2)}}},
        {te, {{66, Info(23)}}},
    });
    out["G22"] = Plan({
        {bs, {{65, Info(21, nullopt, 2)}}},
        {te, {{65, Info(32)}}},
    });
    out["G23"] = Plan({
        {bs, {{65, Info(21, nullopt, 2)}}},
        {te, {{65, Info(32)}}},
    });
    out["G25"] = Plan({
        {bs, {{65, Info(22, nullopt, 2)}}},
        {te, {{66, Info(22)}, {65, Info(22)}, {56, Info(22)}, {55, Info(22)}, {54, Info(22)}, {44, Info(22)}, {33, Info(11)}, {22, Info(11)}}},
    });
    out["G26"] = Plan({
        {bs, {{56, Info(21)}}},
        {te, {{56, Info(21)}}},
    });
    out["G26a"] = Plan({
        {bs, {{66, Info(22, nullopt, 2)}}},
    });
    out["G27"] = Plan({
        {bs, {{65, Info(22, nullopt, 3)}}},
        {te, {{65, Info(21)}}},
    });
    out["G28"] = Plan({
        {bs, {{64, Info(22, nullopt, 4)}}},
        {te, {{64, Info(11)}}},
    });
    out["G31"] = Plan({
        {bs, {{66, Info(22, nullopt, 2)}}},
        {te, {{66, Info(12)}}},
    });
    out["G32"] = Plan({
        {te, {{66, Info(12)}}},
    });
    out["G33"] = Plan({
        {bs, {{66, Info(22, nullopt, 2)}}},
    });
    out["G34"] = Plan({
        {bs, {{56, Info(22, nullopt, 1)}}},
    });
    out["G35"] = Plan({
        {bs, {{65, Info(21, nullopt, 3)}}},
        {te, {{65, Info(32)}}},
    });
    out["G36"] = Plan({
        {bs, {{65, Info(11, nullopt, 4)}}},
        {te, {{65, Info(31)}}},
    });
    out["G37"] = Plan({
        {bs, {{65, Info(11, nullopt, 4)}}},
        {te, {{65, Info(31)}}},
    });
    out["G38"] = Plan({
        {bs, {{66, Info(21, nullopt, 1)}}},
        {te, {{66, Info(32)}}},
    });
    out["G39"] = Plan({
        {bs, {{66, Info(21, nullopt, 1)}}},
        {te, {{66, Info(22)}, {65, Info(22)}, {56, Info(22)}, {55, Info(22)}, {54, Info(22)}, {44, Info(22)}, {43, Info(11)}, {42, Info(11)}, {33, Info(11)}, {22, Info(11)}}},
    });
    out["G41"] = Plan({
        {bs, {{66, Info(12)}}},
    });
    out["G42"] = Plan({
        {bs, {{65, Info(21, nullopt, 3)}}},
        {te, {{65, Info(31)}}},
    });
    out["G43"] = Plan({
        {bs, {{46, Info(11, nullopt, 2)}}},
        {be, {{46, Info(11)}}},
    });
    out["G44"] = Plan({
        {bs, {{66, Info(11, nullopt, 2)}}},
        {te, {{66, Info(21)}}},
    });
    out["G45"] = Plan({
        {bs, {{66, Info(22, nullopt, 2)}}},
    });
    out["G47"] = Plan({
        {bs, {{66, Info(11)}}},
        {te, {{66, Info(31)}}},
    });
    out["G50"] = Plan({
        {bs, {{65, Info(12, nullopt, 2)}}},
    });
    out["G53"] = Plan({
        {te, {{66, Info(23)}}},
    });
    out["I1"] = Plan({
        {bs, {{66, Info(52)}, {65, Info(52)}, {55, Info(42)}, {45, Info(32)}}},
    });
    out["I3"] = Plan({
        {te, {{62, Info(21)}}},
    });
    out["I5"] = Plan({
        {te, {{63, Info(11)}}},
    });
    out["I7"] = Plan({
        {te, {{66, Info(21)}}},
    });
    out["I8"] = Plan({
        {bs, {{56, Info(42)}}},
        {te, {{56, Info(21)}}},
    });
    out["I9"] = Plan({
        {te, {{62, Info(41)}}},
    });
    out["I10"] = Plan({
        {bs, {{66, Info(54)}, {56, Info(43)}, {63, Info(51)}, {33, Info(21)}}},
    });
    out["I10a"] = Plan({
        {bs, {{66, Info(54)}, {56, Info(43)}, {63, Info(51)}, {33, Info(21)}}},
        {ti, {{66, Info(31)}}},
    });
    out["I11"] = Plan({
        {bs, {{66, Info(54)}, {56, Info(43)}, {63, Info(51)}, {33, Info(21)}}},
        {te, {{66, Info(31)}}},
    });
    out["L1"] = Plan({
        {bi, {{46, Info(11)}}},
    });
    out["M9"] = Plan({
        {bs, {{66, Info(52)}}},
    });
    out["M10"] = Plan({
        {bs, {{55, Info(42)}}},
    });
    out["M26"] = Plan({
        {bs, {{46, Info(11)}}},
        {be, {{46, Info(11)}}},
    });
    out["M27"] = Plan({
        {bs, {{66, Info(21)}}},
        {be, {{66, Info(21)}}},
    });
    out["N2"] = Plan({
        {bs, {{66, Info(22)}}},
        {be, {{66, Info(22)}}},
    });
    out["N3"] = Plan({
        {bs, {{56, Info(22)}}},
        {be, {{56, Info(22)}}},
    });
    out["N11"] = Plan({
        {bi, {{62, Info(21)}}},
    });
    out["N36"] = Plan({
        {mi, {{62, Info(31)}}},
    });
    out["N37"] = Plan({
        {mi, {{62, Info(51)}}},
    });
    out["O1"] = Plan({
        {bi, {{53, Info(13)}}},
        {mi, {{53, Info(42)}}},
    });
    out["O6"] = Plan({
        {mi, {{36, Info(42)}}},
    });
    out["O13a"] = Plan({
        {mi, {{66, Info(33, 1, nullopt)}}},
    });
    out["O14"] = Plan({
        {be, {{66, Info(44)}}},
    });
    out["O16"] = Plan({
        {mi, {{64, Info(41)}}},
    });
    out["O17"] = Plan({
        {mi, {{64, Info(31)}}},
    });
    out["O18"] = Plan({
        {mi, {{66, Info(43)}}},
    });
    out["O26"] = Plan({
        {mi, {{46, Info(24)}}},
    });
    out["O32"] = Plan({
        {mi, {{46, Info(13)}}},
        {bi, {{46, Info(13)}}},
    });
    out["O36"] = Plan({
        {mi, {{46, Info(14)}}},
    });
    out["Q2"] = Plan({
        {ts, {{64, Info(32)}}},
    });
    out["R8"] = Plan({
        {bs, {{36, Info(24)}}},
    });
    out["R12"] = Plan({
        {bs, {{64, Info(32)}}},
        {be, {{64, Info(12)}}},
    });
    out["R13"] = Plan({
        {te, {{56, Info(11)}}},
        {be, {{56, Info(12)}}},
    });
    out["S1"] = Plan({
        {ts, {{46, Info(22)}}},
    });
    out["S2"] = Plan({
        {ts, {{66, Info(32)}}},
    });
    out["S22"] = Plan({
        {ti, {{64, Info(21)}}},
        {bi, {{64, Info(31)}}},
    });
    out["S28"] = Plan({
        {bs, {{66, Info(23)}}},
    });
    out["T5"] = Plan({
        {bi, {{66, Info(12)}}},
    });
    out["T6"] = Plan({
        {bs, {{66, Info(21)}}},
    });
    out["T7a"] = Plan({
        {bs, {{36, Info(24)}}},
    });
    out["T14"] = Plan({
        {bs, {{26, Info(14)}}},
    });
    out["T32"] = Plan({
        {bs, {{65, Info(22, nullopt, 4)}}},
    });
    out["U1"] = Plan({
        {ts, {{66, Info(34)}}},
    });
    out["U2"] = Plan({
        {ts, {{65, Info(34)}}},
        {te, {{65, Info(12)}}},
    });
    out["U15"] = Plan({
        {te, {{63, Info(42)}}},
    });
    out["U19"] = Plan({
        {te, {{63, Info(31)}}},
        {ti, {{63, Info(21)}}},
    });
    out["U21"] = Plan({
        {te, {{62, Info(31)}}},
        {ti, {{62, Info(21)}}},
        {bi, {{62, Info(21)}}},
    });
    out["V6"] = Plan({
        {mi, {{23, Info(11)}}},
    });
    out["V7"] = Plan({
        {mi, {{23, Info(11)}}},
    });
    out["V10"] = Plan({
        {mi, {{53, Info(32)}}},
    });
    out["V10n"] = Plan({
        {mi, {{46, Info(34)}}},
    });
    out["V12"] = Plan({
        {bs, {{53, Info(32)}}},
    });
    out["V15"] = Plan({
        {bs, {{65, Info(21, nullopt, 6)}}},
    });
    out["V22"] = Plan({
        {bs, {{64, Info(43)}}},
    });
    out["V23"] = Plan({
        {bs, {{64, Info(43)}}},
    });
    out["V23a"] = Plan({
        {bs, {{66, Info(43)}}},
    });
    out["W4"] = Plan({
        {mi, {{66, Info(32)}}},
    });
    out["Z6"] = Plan({
        {bs, {{64, Info(32)}}},
    });
    out["Z10"] = Plan({
        {ti, {{63, Info(21)}}},
        {bi, {{63, Info(21)}}},
    });
    out["Z11"] = Plan({
        {bs, {{46, Info(12)}}},
        {be, {{46, Info(12)}}},
    });
    out["J7"] = Plan({
        {ts, {{63, Info(41)}}},
        {ti, {{63, Info(21)}}},
    });
    out["J13"] = Plan({
        {mi, {{62, Info(21)}}},
    });
    out["J15"] = Plan({
        {mi, {{62, Info(21)}}},
    });
    out["J19"] = Plan({
        {mi, {{44, Info(12)}}},
        {bi, {{44, Info(12)}}},
    });
    return Status::Ok();
  }
};

} // namespace eglyf

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

  struct Info {
    explicit Info(WxH size, std::optional<int> dx = std::nullopt, std::optional<int> dy = std::nullopt) : size(size), dx(dx), dy(dy) {}
    WxH size;
    std::optional<int> dx;
    std::optional<int> dy;
  };

  struct Plan {
    std::map<Pos, std::map<WxH, Info>> insertions;
  };

  static Status CreatePlan(FontFile const &font, std::map<std::string, Plan> &out) {
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
        {be, {{66, Info(11)}}},
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
        {be, {{56, Info(22)}}},
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
        {be, {{65, Info(21)}}},
    });
    out["G25"] = Plan({
        {bs, {{65, Info(22, nullopt, 2)}}},
        {te, {{66, Info(22)}, {65, Info(22)}, {56, Info(22)}, {55, Info(22)}, {54, Info(22)}, {44, Info(22)}, {33, Info(11)}, {22, Info(11)}}},
        {be, {{65, Info(21)}}},
    });
    out["G26"] = Plan({
        {bs, {{56, Info(21)}}},
        {te, {{56, Info(21)}}},
        {be, {{56, Info(21)}}},
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
        {be, {{65, Info(21)}, {44, Info(11)}, {42, Info(11)}, {22, Info(11)}}},
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
        {te, {{66, Info(31)}, {65, Info(31)}, {64, Info(21)}, {56, Info(31)}, {55, Info(31)}, {54, Info(31)}, {46, Info(21)}, {45, Info(21)}, {44, Info(21)}, {36, Info(11)}, {35, Info(11)}, {34, Info(11)}}},
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

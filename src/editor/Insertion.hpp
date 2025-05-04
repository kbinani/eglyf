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
    Info() = default;
    explicit Info(WxH size, std::optional<int> dx = std::nullopt, std::optional<int> dy = std::nullopt) : size(size), dx(dx), dy(dy) {}
    WxH size;
    std::optional<int> dx;
    std::optional<int> dy;
  };

  struct Plan {
    std::map<Pos, std::map<WxH, Info>> insertions;
  };

  static Status CreatePlan(FontFile const &font,
                           std::unordered_map<std::string, SizeVariants> const &sizeVariants,
                           int16_t chu,
                           int16_t vhu,
                           int16_t hfu,
                           int16_t vfu,
                           int16_t base,
                           int16_t lineWidth,
                           int insertionResolution,
                           std::map<std::string, Plan> &out) {
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

    for (auto const &[name, poss] : positions) {
      auto found = sizeVariants.find(name);
      if (found == sizeVariants.end()) {
        continue;
      }
      WxH baseSize = found->second.size;
      for (Pos pos : poss) {
        auto result = ScanInsertionSpot(font, name, baseSize, pos, chu, vhu, hfu, vfu, base, lineWidth, insertionResolution);
        if (result) {
          WxH sz = result->first;
          Vec<int16_t> offset = result->second;
          Info info(sz);
          if (offset.x != 0) {
            info.dx = offset.x;
          }
          if (offset.y != 0) {
            info.dy = offset.y;
          }
          out[name].insertions[pos][baseSize] = info;
        }
        for (auto i = found->second.variants.begin(); i != found->second.variants.end(); i++) {
          WxH variantSize = i->first;
          shared_ptr<Glyph> g = i->second;
          auto r = ScanInsertionSpot(font, g->name, variantSize, pos, chu, vhu, hfu, vfu, base, lineWidth, insertionResolution);
          if (r) {
            WxH sz = r->first;
            Vec<int16_t> offset = r->second;
            Info info(sz);
            if (offset.x != 0) {
              info.dx = offset.x;
            }
            if (offset.y != 0) {
              info.dy = offset.y;
            }
            out[name].insertions[pos][variantSize] = info;
          }
        }
      }
    }

    return Status::Ok();
  }

  static std::optional<std::pair<WxH, Vec<int16_t>>> ScanInsertionSpot(FontFile const &font,
                                                                       std::string const &name,
                                                                       WxH size,
                                                                       Pos pos,
                                                                       int16_t chu,
                                                                       int16_t vhu,
                                                                       int16_t hfu,
                                                                       int16_t vfu,
                                                                       int16_t base,
                                                                       int16_t lineWidth,
                                                                       int insertionResolution) {
    using namespace std;
    int const res = insertionResolution;
    double const xres = hfu * chu / double(res);
    double const yres = vfu * vhu / double(res);
    int const margin = lineWidth / 2;

    if (!holds_alternative<FontFile::TrueTypeOutlines>(font.outlines)) {
      return nullopt;
    }
    auto const &outlines = get<FontFile::TrueTypeOutlines>(font.outlines);
    auto const &glyf = outlines.glyf;
    auto gid = font.postGetGlyphID(name);
    if (!gid) {
      return nullopt;
    }
    Shape shape;
    if (auto st = glyf->toShape(*gid, shape); !st.ok()) {
      return nullopt;
    }
    int wO = WidthFromWxH(size);
    int hO = HeightFromWxH(size);
    int xMax = wO - 1;
    int yMax = hO - 1;
    if (xMax < 1 || yMax < 1) {
      return nullopt;
    }
    int width = WidthFromWxH(size) * hfu;
    int height = HeightFromWxH(size) * vfu;
    int x0 = -width / 2;
    int y0 = base - margin;
    int x1 = x0 + width;
    int y1 = y0 + height;
    deque<pair<WxH, Vec<int16_t>>> ok;
    for (int x = 1; x <= xMax; x++) {
      for (int y = 1; y <= yMax; y++) {
        int w = x * hfu;
        int h = y * vfu;
        int left;
        int bottom;
        Rect<int16_t> limit;
        switch (pos) {
        case Pos::TopStart:
          left = x0;
          bottom = y1 - h;
          limit = Rect<int16_t>(x0, max(y0 + height / 2 - h, y0), min(x0 + width / 2 + w, x1), y1);
          break;
        case Pos::BottomStart:
          left = x0;
          bottom = y0;
          limit = Rect<int16_t>(x0, y0, min(x0 + width / 2 + w, x1), min(y0 + height / 2 + h, y1));
          break;
        case Pos::TopEnd:
          left = x0 + width - w;
          bottom = y0 + height - h;
          limit = Rect<int16_t>(max(x0 + width / 2 - w, x0), max(y0 + height / 2 - h, y0), x1, y1);
          break;
        case Pos::BottomEnd:
          left = x0 + width - w;
          bottom = y0;
          limit = Rect<int16_t>(max(x0 + width / 2 - w, x0), y0, x1, min(y0 + height / 2 - h, y0));
          break;
        case Pos::Top:
          left = x0 + width / 2 - w / 2;
          bottom = y0 + height - h;
          limit = Rect<int16_t>(max(x0 + width / 2 - w, x0), max(y0 + height / 2 - h, y0), min(x0 + width / 2 + w, x1), y1);
          break;
        case Pos::Middle:
          left = x0 + width / 2 - w / 2;
          bottom = y0 + height / 2 - h / 2;
          limit = Rect<int16_t>(max(x0 + width / 2 - w, x0), max(y0 + height / 2 - h, y0), min(x0 + width / 2 + w, x1), min(y0 + height / 2 + h, y1));
          break;
        case Pos::Bottom:
          left = x0 + width / 2 - w / 2;
          bottom = y0;
          limit = Rect<int16_t>(max(x0 + width / 2 - w, x0), y0, min(x0 + width / 2 + w, x1), min(y0 + height / 2 + h, y1));
          break;
        }
        Rect<double> rect(left, bottom, left + w, bottom + h);
        WxH sz = x * 10 + y;
        if (!shape.intersects(rect)) {
          ok.push_back(make_pair(sz, Vec<int16_t>(0, 0)));
        }
        int ix0 = (int)floor((x0 - left) / xres);
        int ix1 = (int)ceil((x1 - (left + w)) / yres);
        int iy0 = (int)floor((y0 - bottom) / xres);
        int iy1 = (int)ceil((y1 - (bottom + h)) / yres);
        for (int ix = ix0; ix <= ix1; ix++) {
          for (int iy = iy0; iy <= iy1; iy++) {
            Rect<double> sub(left + ix * xres, bottom + iy * yres, left + ix * xres + w, bottom + iy * yres + h);
            if (limit.xMin <= sub.xMin && sub.xMax <= limit.xMax && limit.yMin <= sub.yMin && sub.yMax <= limit.yMax) {
              if (!shape.intersects(sub)) {
                ok.push_back(make_pair(sz, Vec<int16_t>(ix, iy)));
              }
            }
          }
        }
      }
    }
    if (ok.empty()) {
      return nullopt;
    } else if (ok.size() == 1) {
      return ok.front();
    }
    ranges::sort(ok, [=](auto const &a, auto const &b) {
      int sA = AreaFromWxH(a.first);
      int sB = AreaFromWxH(b.first);
      if (sA == sB) {
        double aA = AspectRatioFromWxH(a.first);
        double aB = AspectRatioFromWxH(b.first);
        double aO = wO / (double)hO;
        return fabs(aA - aO) < fabs(aB - aO);
      } else {
        return sA > sB;
      }
    });

    pair<WxH, Vec<int16_t>> front = ok.front();
    ok.erase(ranges::remove_if(ok, [&](auto const &it) { return AreaFromWxH(it.first) != AreaFromWxH(front.first); }).begin(), ok.end());
    if (ok.size() == 1) {
      return front;
    }

    int xSign = 0;
    int ySign = 0;
    switch (pos) {
    case Pos::TopStart:
      xSign = -1;
      ySign = 1;
      break;
    case Pos::BottomStart:
      xSign = -1;
      ySign = -1;
      break;
    case Pos::TopEnd:
      xSign = 1;
      ySign = 1;
      break;
    case Pos::BottomEnd:
      xSign = 1;
      ySign = -1;
      break;
    case Pos::Top:
      xSign = 0;
      ySign = 1;
      break;
    case Pos::Middle:
      xSign = 0;
      ySign = 0;
      break;
    case Pos::Bottom:
      xSign = 0;
      ySign = -1;
      break;
    }
    ranges::sort(ok, [=](auto const &a, auto const &b) {
      Vec<int16_t> va = a.second;
      Vec<int16_t> vb = b.second;
      switch (xSign) {
      case 0:
        if (abs(va.x) != abs(vb.x)) {
          return abs(va.x) < abs(vb.x);
        }
        break;
      case 1:
        if (va.x != vb.x) {
          return va.x > vb.x;
        }
        break;
      case -1:
        if (va.x != vb.x) {
          return va.x < vb.x;
        }
        break;
      }
      switch (ySign) {
      case 0:
        if (abs(va.y) != abs(vb.y)) {
          return abs(va.y) < abs(vb.y);
        }
        break;
      case 1:
        if (va.y != vb.y) {
          return va.y > vb.y;
        }
        break;
      case -1:
        if (va.y != vb.y) {
          return va.y < vb.y;
        }
        break;
      }
      return false;
    });
    pair<WxH, Vec<int16_t>> best = ok.front();
    return best;
  }
};

} // namespace eglyf

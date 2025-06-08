static eglyf::Status Expand(eglyf::glyf::GlyphDataTable const &glyf,
                            eglyf::glyf::GlyphDataTable::CompositeGlyph const &cg,
                            eglyf::Transform<int16_t> mtx,
                            std::vector<eglyf::glyf::GlyphDataTable::Contour> &contours) {
  using namespace std;
  using namespace eglyf;
  for (auto const &r : cg.records) {
    auto txm = Transform<int16_t>::Concat(mtx, r.transform<int16_t>());
    auto g = glyf.glyphs[r.glyphIndex];
    if (holds_alternative<glyf::GlyphDataTable::EmptyGlyph>(g)) {
      continue;
    } else if (holds_alternative<glyf::GlyphDataTable::ReadonlyGlyph>(g)) {
      auto rg = get<glyf::GlyphDataTable::ReadonlyGlyph>(g);
      auto sg = rg.toSimpleGlyph();
      if (!sg) {
        return EGLYF_STATUS_PUSH(sg.status());
      }
      for (auto const &c : sg->contours) {
        auto tc = c.transformed(txm);
        contours.push_back(tc);
      }
    } else if (holds_alternative<glyf::GlyphDataTable::SimpleGlyph>(g)) {
      return EGLYF_ERROR;
    } else if (holds_alternative<glyf::GlyphDataTable::CompositeGlyph>(g)) {
      auto cg = get<glyf::GlyphDataTable::CompositeGlyph>(g);
      auto st = Expand(glyf, cg, txm, contours);
      if (!st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    } else {
      return EGLYF_ERROR;
    }
  }
  return Status::Ok();
}

TEST_CASE("tuffy") {
  using namespace std;
  using namespace eglyf;
  namespace fs = std::filesystem;
  FileInputStream fis(fs::path("Tuffy.ttf"));
  shared_ptr<Font> font;
  REQUIRE(Font::Read(fis, font).ok());
  REQUIRE(holds_alternative<Font::TrueTypeOutlines>(font->outlines));
  auto const &outline = get<Font::TrueTypeOutlines>(font->outlines);
  auto const &glyf = outline.glyf;
  size_t empty = 0;
  size_t simple = 0;
  size_t composite = 0;

  for (uint32_t code = 32; code <= 126; code++) {
    auto gid = font->getGlyphID(code);
    if (!gid) {
      continue;
    }
    auto const &glyph = glyf->glyphs[*gid];
    if (holds_alternative<glyf::GlyphDataTable::EmptyGlyph>(glyph)) {
      empty++;
      FileOutputStream fos(fs::path(format("res/{}.glyf", code)));
    } else if (holds_alternative<glyf::GlyphDataTable::ReadonlyGlyph>(glyph)) {
      simple++;
      auto const &rg = get<glyf::GlyphDataTable::ReadonlyGlyph>(glyph);
      auto sg = rg.toSimpleGlyph();
      REQUIRE(sg);
      ByteOutputStream out;
      REQUIRE(sg->encode(out).ok());

      string data = out.data();
      FileOutputStream fos(fs::path(format("res/{}.glyf", code)));
      REQUIRE(fos.write(data.data(), data.size()));
    } else if (holds_alternative<glyf::GlyphDataTable::CompositeGlyph>(glyph)) {
      composite++;
      auto cg = get<glyf::GlyphDataTable::CompositeGlyph>(glyph);
      glyf::GlyphDataTable::SimpleGlyph sg;
      auto st = Expand(*glyf, cg, Transform<int16_t>(), sg.contours);
      REQUIRE(st.ok());
      REQUIRE(sg.contours.size() <= numeric_limits<int16_t>::max());
      sg.header.numberOfContours = sg.contours.size();
      optional<Rect<int16_t>> bounds;
      size_t numPoints = 0;
      for (auto const &c : sg.contours) {
        auto b = c.boundingBox();
        if (bounds) {
          bounds->updateBound(b.xMin, b.yMin, b.xMax, b.yMax);
        } else {
          bounds = b;
        }
        numPoints += c.points.size();
      }
      REQUIRE(bounds);
      sg.header.xMin = bounds->xMin;
      sg.header.yMin = bounds->yMin;
      sg.header.xMax = bounds->xMax;
      sg.header.yMax = bounds->yMax;
      REQUIRE(numPoints <= numeric_limits<uint16_t>::max());
      sg.numPoints = numPoints;
      ByteOutputStream out;
      REQUIRE(sg.encode(out).ok());
      string data = out.data();
      FileOutputStream fos(fs::path(format("res/{}.glyf", code)));
      REQUIRE(fos.write(data.data(), data.size()));
    } else {
      REQUIRE(false);
    }
  }
  cout << "empty=" << empty << "; simple=" << simple << "; composite=" << composite << endl;
}

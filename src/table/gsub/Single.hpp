#pragma once

namespace eglyf::gsub {

class SingleFormat1 : public Subtable {
public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return EGLYF_ERROR;
    }
    auto r = make_unique<SingleFormat1>();
    if (!in.i16(&r->deltaGlyphID)) {
      return EGLYF_ERROR;
    }
    if (!in.seek(coverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, r->coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &) override {
    using namespace std;
    auto beginning = make_shared<OffsetWriter>(out);
    if (!out.u16(1)) {
      return EGLYF_ERROR;
    }
    auto coverageOffset = beginning->o16();
    if (!coverageOffset) {
      return EGLYF_ERROR;
    }
    if (!out.i16(deltaGlyphID)) {
      return EGLYF_ERROR;
    }
    if (auto st = coverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return EGLYF_STATUS_PUSH(beginning->commit());
  }

public:
  int16_t deltaGlyphID;
};

class SingleFormat2 : public Subtable {
public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return EGLYF_ERROR;
    }
    uint16_t glyphCount;
    if (!in.u16(&glyphCount)) {
      return EGLYF_ERROR;
    }
    auto r = make_unique<SingleFormat2>();
    if (!in.u16a(r->substituteGlyphIDs, glyphCount)) {
      return EGLYF_ERROR;
    }
    if (!in.seek(coverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, r->coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &) override {
    using namespace std;
    auto beginning = make_shared<OffsetWriter>(out);
    if (!out.u16(2)) {
      return EGLYF_ERROR;
    }
    auto coverageOffset = beginning->o16();
    if (!coverageOffset) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(substituteGlyphIDs.size())) {
      return EGLYF_ERROR;
    }
    if (!out.u16a(substituteGlyphIDs)) {
      return EGLYF_ERROR;
    }
    if (auto st = coverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return EGLYF_STATUS_PUSH(beginning->commit());
  }

public:
  std::vector<uint16_t> substituteGlyphIDs;
};

class Single {
  Single() = delete;

public:
  static Status Read(InputStream &stream, std::shared_ptr<Subtable> &out) {
    using namespace std;
    OffsetInputStream in(&stream);
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    if (format == 1) {
      return EGLYF_STATUS_PUSH(SingleFormat1::Read(in, out));
    } else if (format == 2) {
      return EGLYF_STATUS_PUSH(SingleFormat2::Read(in, out));
    } else {
      return EGLYF_ERROR;
    }
  }
};

} // namespace eglyf::gsub

#pragma once

namespace eglyf::gpos {

class Anchor {
public:
  virtual ~Anchor() {}
};

class Anchor1 : public Anchor {
public:
  static Status Read(InputStream &in, std::shared_ptr<Anchor> &out) {
    using namespace std;
    auto ret = make_unique<Anchor1>();
    if (!in.i16(&ret->xCoordinate)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&ret->yCoordinate)) {
      return EGLYF_ERROR;
    }
    out.reset(ret.release());
    return Status::Ok();
  }

public:
  int16_t xCoordinate;
  int16_t yCoordinate;
};

class Anchor2 : public Anchor {
public:
  static Status Read(InputStream &in, std::shared_ptr<Anchor> &out) {
    using namespace std;
    auto ret = make_unique<Anchor2>();
    if (!in.i16(&ret->xCoordinate)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&ret->yCoordinate)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&ret->anchorPoint)) {
      return EGLYF_ERROR;
    }
    out.reset(ret.release());
    return Status::Ok();
  }

public:
  int16_t xCoordinate;
  int16_t yCoordinate;
  uint16_t anchorPoint;
};

class AnchorReader {
  AnchorReader() = delete;

public:
  static Status Read(InputStream &in, std::shared_ptr<Anchor> &out) {
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    switch (format) {
    case 1:
      return EGLYF_STATUS_PUSH(Anchor1::Read(in, out));
    case 2:
      return EGLYF_STATUS_PUSH(Anchor2::Read(in, out));
    case 3:
      return EGLYF_ERROR_WHAT("Unimplemented AnchorFormat3");
    default:
      return EGLYF_ERROR;
    }
  }
};

} // namespace eglyf::gpos

#pragma once

namespace eglyf::cff {

class Index {
public:
  static Status Read(InputStream &in, std::shared_ptr<Index> &out) {
    using namespace std;
    Card16 count;
    if (!in.u16(&count)) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<Index>();
    if (count == 0) {
      out.reset(ret.release());
      return Status::Ok();
    }
    OffSize offSize;
    if (!in.u8(&offSize)) {
      return EGLYF_ERROR;
    }
    if (offSize == 0 || offSize > 4) {
      return EGLYF_ERROR;
    }
    vector<uint32_t> offset;
    array<uint8_t, 4> buffer;
    for (size_t i = 0; i <= count; i++) {
      if (in.read(buffer.data(), offSize) != offSize) {
        return EGLYF_ERROR;
      }
      uint32_t v = 0;
      for (int j = 0; j < offSize; j++) {
        v = v * 256 + buffer[j];
      }
      if (v == 0) {
        return EGLYF_ERROR;
      }
      offset.push_back(v - 1);
    }
    OffsetInputStream ois(&in);
    for (size_t i = 0; i < count; i++) {
      uint32_t from = offset[i];
      uint32_t to = offset[i + 1];
      if (from > to) {
        return EGLYF_ERROR;
      }
      if (!ois.seek(from)) {
        return EGLYF_ERROR;
      }
      string data;
      data.resize(to - from);
      if (data.size() != ois.read(data.data(), data.size())) {
        return EGLYF_ERROR;
      }
      ret->data.push_back(data);
    }
    out.reset(ret.release());
    return Status::Ok();
  }

public:
  std::vector<std::string> data;
};

} // namespace eglyf::cff

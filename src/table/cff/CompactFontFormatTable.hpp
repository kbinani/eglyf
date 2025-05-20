#pragma once

namespace eglyf::cff {

class CompactFontFormatTable : public Table {
public:
  struct Header {
    Card8 major;
    Card8 minor;
    Card8 hdrSize;
    OffSize offSize;

    Status read(InputStream &in) {
      if (!in.u8(&major)) {
        return EGLYF_ERROR;
      }
      if (!in.u8(&minor)) {
        return EGLYF_ERROR;
      }
      if (major != 1 || minor != 0) {
        return EGLYF_ERROR;
      }
      if (!in.u8(&hdrSize)) {
        return EGLYF_ERROR;
      }
      if (!in.u8(&offSize)) {
        return EGLYF_ERROR;
      }
      return Status::Ok();
    }
  };

public:
  Optional<EncodeResult> encode() const override {
    return EGLYF_NULLOPT;
  }

  static Status Read(InputStream &stream, std::shared_ptr<CompactFontFormatTable> &out) {
    using namespace std;
    OffsetInputStream in(&stream);
    auto ret = make_unique<CompactFontFormatTable>();
    if (auto st = ret->header.read(in); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (!in.seek(ret->header.hdrSize)) {
      return EGLYF_ERROR;
    }
    if (auto st = Index::Read(in, ret->nameIndex); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    shared_ptr<Index> topDictIndex;
    if (auto st = Index::Read(in, topDictIndex); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (topDictIndex->data.size() != 1) {
      return EGLYF_ERROR;
    }
    auto const &data = topDictIndex->data[0];
    ret->topDict = make_shared<Dict>();
    if (auto st = ret->topDict->read(data); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = Index::Read(in, ret->stringIndex); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = Index::Read(in, ret->globalSubrIndex); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    vector<int32_t> privateSizeAndOffset;
    if (ret->topDict->i32a(privateSizeAndOffset, 18)) {
      if (privateSizeAndOffset.size() != 2) {
        return EGLYF_ERROR;
      }
      int32_t size = privateSizeAndOffset[0];
      int32_t offset = privateSizeAndOffset[1];
      if (size < 0) {
        return EGLYF_ERROR;
      }
      if (offset < 0) {
        return EGLYF_ERROR;
      }
      ret->privateDict = make_shared<Dict>();
      if (!in.seek(offset)) {
        return EGLYF_ERROR;
      }
      string data;
      data.resize(size);
      if (in.read(data.data(), data.size()) != data.size()) {
        return EGLYF_ERROR;
      }
      if (auto st = ret->privateDict->read(data); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }

      auto subrs = ret->privateDict->i32(19);
      if (subrs) {
        if (!in.seek(offset + *subrs)) {
          return EGLYF_ERROR;
        }
        if (auto st = Index::Read(in, ret->localSubrIndex); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
    }

    auto fdArrayOffset = ret->topDict->i32(12, 36);
    auto fdSelectOffset = ret->topDict->i32(12, 37);
    if ((bool)fdArrayOffset != (bool)fdSelectOffset) {
      return EGLYF_ERROR;
    }

    auto charStringsOffset = ret->topDict->i32(17);
    if (charStringsOffset && fdArrayOffset) {
      return EGLYF_ERROR;
    }

    bool const isCID = (bool)fdArrayOffset;

    if (isCID) {
      // TODO:
    } else {
      if (!in.seek(*charStringsOffset)) {
        return EGLYF_ERROR;
      }
      if (auto st = Index::Read(in, ret->charStringsIndex); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    return EGLYF_ERROR;
  }

  std::optional<std::string> str(SID id) const {
    using namespace std;
    auto size = StdStrings::Size();
    if (id < size) {
      return StdStrings::Get(id);
    } else {
      size_t index = id - size;
      if (index < stringIndex->data.size()) {
        return stringIndex->data[index];
      } else {
        return nullopt;
      }
    }
  }

public:
  Header header;
  std::shared_ptr<Index> nameIndex;
  std::shared_ptr<Dict> topDict;
  std::shared_ptr<Index> stringIndex;
  std::shared_ptr<Index> globalSubrIndex;
  std::shared_ptr<Dict> privateDict;
  std::shared_ptr<Index> localSubrIndex;
  std::shared_ptr<Index> charStringsIndex;
};

} // namespace eglyf::cff

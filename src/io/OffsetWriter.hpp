#pragma once

namespace eglyf {

class OffsetWriter : public std::enable_shared_from_this<OffsetWriter> {
public:
  template <class T>
  class Handle {
  public:
    int64_t const position;
    std::optional<std::pair<int64_t, T>> result;
    std::weak_ptr<OffsetWriter> const parent;

    explicit Handle(int64_t position, std::shared_ptr<OffsetWriter> parent) : position(position), parent(parent) {}

    Status mark() {
      using namespace std;
      if (result) {
        return EGLYF_ERROR;
      }
      auto p = parent.lock();
      if (!p) {
        return EGLYF_ERROR;
      }
      int64_t latest = p->upstream.position();
      int64_t offset = latest - p->base;
      if (offset < (int64_t)numeric_limits<T>::lowest() || (int64_t)numeric_limits<T>::max() < offset) {
        return EGLYF_ERROR;
      }
      result = make_pair(position, (T)offset);
      return Status::Ok();
    }

    Status null() {
      using namespace std;
      if (result) {
        return EGLYF_ERROR;
      }
      result = make_pair(position, 0);
      return Status::Ok();
    }
  };

  using Handle16 = std::shared_ptr<Handle<Offset16>>;
  using Handle32 = std::shared_ptr<Handle<Offset32>>;

public:
  explicit OffsetWriter(OutputStream &upstream) : upstream(upstream), base(upstream.position()) {
  }

  Handle16 o16() {
    using namespace std;
    int64_t const pos = upstream.position();
    if (auto found = offsets.find(pos); found != offsets.end()) {
      return nullptr;
    }
    if (!upstream.o16(0)) {
      return nullptr;
    }
    auto handle = make_shared<Handle<Offset16>>(pos, shared_from_this());
    offsets[pos] = handle;
    return handle;
  }

  Handle32 o32() {
    using namespace std;
    int64_t const pos = upstream.position();
    if (auto found = offsets.find(pos); found != offsets.end()) {
      return nullptr;
    }
    if (!upstream.o32(0)) {
      return nullptr;
    }
    auto handle = make_shared<Handle<Offset32>>(pos, shared_from_this());
    offsets[pos] = handle;
    return handle;
  }

  Status commit() {
    using namespace std;
    if (offsets.empty()) {
      return Status::Ok();
    }
    auto pos = upstream.position();
    for (auto const &[position, handle] : offsets) {
      if (holds_alternative<Handle16>(handle)) {
        auto const &h16 = get<Handle16>(handle);
        if (!h16->result) {
          return EGLYF_ERROR;
        }
        if (!upstream.seek(position)) {
          return EGLYF_ERROR;
        }
        if (!upstream.o16(h16->result->second)) {
          return EGLYF_ERROR;
        }
      } else if (holds_alternative<Handle32>(handle)) {
        auto const &h32 = get<Handle32>(handle);
        if (!h32->result) {
          return EGLYF_ERROR;
        }
        if (!upstream.seek(position)) {
          return EGLYF_ERROR;
        }
        if (!upstream.o32(h32->result->second)) {
          return EGLYF_ERROR;
        }
      } else {
        return EGLYF_ERROR;
      }
    }
    if (upstream.seek(pos)) {
      return Status::Ok();
    } else {
      return EGLYF_ERROR;
    }
  }

private:
  OutputStream &upstream;
  int64_t const base;
  std::map<int64_t, std::variant<Handle16, Handle32>> offsets;
};

} // namespace eglyf

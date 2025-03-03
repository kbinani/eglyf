#pragma once

namespace eglyf {

class OffsetWriter : public std::enable_shared_from_this<OffsetWriter> {
public:
  template <class T>
  class Handle {
  public:
    int64_t const position;
    std::optional<std::pair<int64_t, T>> result;
    std::weak_ptr<OffsetWriter> parent;

    explicit Handle(int64_t position, std::shared_ptr<OffsetWriter> parent) : position(position), parent(parent) {}

    bool mark() {
      using namespace std;
      if (result) {
        return false;
      }
      auto p = parent.lock();
      if (!p) {
        return false;
      }
      int64_t latest = p->upstream.position();
      int64_t offset = latest - p->base;
      if (offset < (int64_t)numeric_limits<T>::lowest() || (int64_t)numeric_limits<T>::max() < offset) {
        return false;
      }
      result = make_pair(position, (T)offset);
      return true;
    }
  };

public:
  explicit OffsetWriter(OutputStream &upstream) : upstream(upstream), base(upstream.position()) {
  }

  std::shared_ptr<Handle<Offset16>> o16() {
    using namespace std;
    auto pos = upstream.position();
    if (offsets.count(pos) > 0) {
      return nullptr;
    }
    if (!upstream.o16(0)) {
      return nullptr;
    }
    auto handle = make_shared<Handle<Offset16>>(upstream.position(), shared_from_this());
    offsets[pos] = handle;
    return handle;
  }

  std::shared_ptr<Handle<Offset32>> o32() {
    using namespace std;
    auto pos = upstream.position();
    if (offsets.count(pos) > 0) {
      return nullptr;
    }
    if (!upstream.o32(0)) {
      return nullptr;
    }
    auto handle = make_shared<Handle<Offset32>>(upstream.position(), shared_from_this());
    offsets[pos] = handle;
    return handle;
  }

  bool commit() {
    using namespace std;
    if (offsets.empty()) {
      return true;
    }
    auto pos = upstream.position();
    for (auto const &[position, handle] : offsets) {
      if (holds_alternative<shared_ptr<Handle<Offset16>>>(handle)) {
        auto const &h16 = get<shared_ptr<Handle<Offset16>>>(handle);
        if (!h16->result) {
          return false;
        }
        if (!upstream.seek(position)) {
          return false;
        }
        if (!upstream.o16(h16->result->second)) {
          return false;
        }
      } else if (holds_alternative<shared_ptr<Handle<Offset32>>>(handle)) {
        auto const &h32 = get<shared_ptr<Handle<Offset32>>>(handle);
        if (!h32->result) {
          return false;
        }
        if (!upstream.seek(position)) {
          return false;
        }
        if (!upstream.o32(h32->result->second)) {
          return false;
        }
      } else {
        return false;
      }
    }
    return upstream.seek(pos);
  }

private:
  OutputStream &upstream;
  int64_t const base;
  std::map<int64_t, std::variant<std::shared_ptr<Handle<Offset16>>, std::shared_ptr<Handle<Offset32>>>> offsets;
};

} // namespace eglyf

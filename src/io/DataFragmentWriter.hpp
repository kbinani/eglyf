#pragma once

namespace eglyf {

class DataFragmentWriter : public std::enable_shared_from_this<DataFragmentWriter>, public OutputStream {
public:
  template <std::unsigned_integral T>
  class Marker {
  public:
    Marker() = default;
  };

  using Marker16 = std::shared_ptr<Marker<Offset16>>;
  using Marker32 = std::shared_ptr<Marker<Offset32>>;

private:
  struct HeaderFragment {
    std::string data;
    std::map<int64_t, std::variant<Marker16, Marker32>> markers;
  };

  struct DataFragment {
    std::string data;
    std::vector<std::variant<Marker16, Marker32>> markers;
  };

  struct ActiveHeaderFragment {
    ByteOutputStream stream;
    std::map<int64_t, std::variant<Marker16, Marker32>> markers;
  };

  struct ActiveDataFragment {
    ByteOutputStream stream;
    std::vector<std::variant<Marker16, Marker32>> markers;
  };

  using DataFragmentPtr = std::shared_ptr<DataFragment>;

public:
  explicit DataFragmentWriter(OutputStream *upstream) : upstream(upstream), base(upstream->position()) {
    using namespace std;
    active = ActiveHeaderFragment();
  }

  bool write(void const *buffer, size_t size) override {
    if (holds_alternative<ActiveHeaderFragment>(active)) {
      auto &a = get<ActiveHeaderFragment>(active);
      return a.stream.write(buffer, size);
    } else if (holds_alternative<ActiveDataFragment>(active)) {
      auto &a = get<ActiveDataFragment>(active);
      return a.stream.write(buffer, size);
    } else {
      return false;
    }
  }

  bool seek(int64_t loc) override {
    if (holds_alternative<ActiveHeaderFragment>(active)) {
      auto &a = get<ActiveHeaderFragment>(active);
      return a.stream.seek(loc);
    } else if (holds_alternative<ActiveDataFragment>(active)) {
      auto &a = get<ActiveDataFragment>(active);
      return a.stream.seek(loc);
    } else {
      return false;
    }
  }

  int64_t position() override {
    if (holds_alternative<ActiveHeaderFragment>(active)) {
      auto &a = get<ActiveHeaderFragment>(active);
      return a.stream.position();
    } else if (holds_alternative<ActiveDataFragment>(active)) {
      auto &a = get<ActiveDataFragment>(active);
      return a.stream.position();
    } else {
      return -1;
    }
  }

  Marker16 o16() {
    using namespace std;
    if (!holds_alternative<ActiveHeaderFragment>(active)) {
      return nullptr;
    }
    auto &hf = get<ActiveHeaderFragment>(active);
    int64_t const pos = hf.stream.position();
    if (auto found = hf.markers.find(pos); found != hf.markers.end()) {
      return nullptr;
    }
    if (!hf.stream.o16(0)) {
      return nullptr;
    }
    auto handle = make_shared<Marker<Offset16>>();
    hf.markers[pos] = handle;
    return handle;
  }

  Marker32 o32() {
    using namespace std;
    if (!holds_alternative<ActiveHeaderFragment>(active)) {
      return nullptr;
    }
    auto &hf = get<ActiveHeaderFragment>(active);
    int64_t const pos = hf.stream.position();
    if (auto found = hf.markers.find(pos); found != hf.markers.end()) {
      return nullptr;
    }
    if (!hf.stream.o32(0)) {
      return nullptr;
    }
    auto handle = make_shared<Marker<Offset32>>();
    hf.markers[pos] = handle;
    return handle;
  }

  Status startDataFragment(std::vector<Marker16> const &markers) {
    using namespace std;
    if (holds_alternative<ActiveDataFragment>(active)) {
      auto &df = get<ActiveDataFragment>(active);
      auto data = df.stream.data();
      auto f = make_shared<DataFragment>();
      f->data = data;
      ranges::copy(df.markers, back_inserter(f->markers));
      fragments.push_back(f);
    } else if (holds_alternative<ActiveHeaderFragment>(active)) {
      auto &hf = get<ActiveHeaderFragment>(active);
      auto data = hf.stream.data();
      if (!data.empty()) {
        HeaderFragment f;
        f.data = data;
        for (auto const &it : hf.markers) {
          f.markers.insert(it);
        }
        fragments.push_back(f);
      }
    }

    ActiveDataFragment next;
    ranges::copy(markers, back_inserter(next.markers));
    active = next;
    return Status::Ok();
  }

  Status endDataFragment() {
    using namespace std;
    if (!holds_alternative<ActiveDataFragment>(active)) {
      return EGLYF_ERROR;
    }
    auto &a = get<ActiveDataFragment>(active);
    auto data = a.stream.data();
    if (auto found = lut.find(data); found == lut.end()) {
      assert(!data.empty());
      auto f = make_shared<DataFragment>();
      f->data = data;
      ranges::copy(a.markers, back_inserter(f->markers));
      fragments.push_back(f);
      lut[data] = f;
    } else {
      auto &f = found->second;
      assert(f->data == data);
      ranges::copy(a.markers, back_inserter(f->markers));
      fragments.push_back(f);
    }

    ActiveHeaderFragment next;
    active = next;
    return Status::Ok();
  }

  template <class T>
    requires requires(T const &data, OutputStream &out) {
      { data.write(out) } -> std::convertible_to<Status>;
    }
  Status writeDataFragment(std::vector<Marker16> const &markers, T const &data) {
    if (auto st = startDataFragment(markers); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (Status st = data.write(*this); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return EGLYF_STATUS_PUSH(endDataFragment());
  }

  Status commit() {
    using namespace std;
    if (holds_alternative<ActiveHeaderFragment>(active)) {
      auto &a = get<ActiveHeaderFragment>(active);
      auto data = a.stream.data();
      if (!data.empty()) {
        HeaderFragment f;
        f.data = data;
        for (auto const &it : a.markers) {
          f.markers.insert(it);
        }
        fragments.push_back(f);
      }
    } else if (holds_alternative<ActiveDataFragment>(active)) {
      return EGLYF_ERROR;
    }

    int64_t const begin = upstream->position();
    map<DataFragmentPtr, int> usage;
    for (auto const &f : fragments) {
      if (holds_alternative<DataFragmentPtr>(f)) {
        auto const &df = get<DataFragmentPtr>(f);
        usage[df] += 1;
      }
    }
    map<Marker16, int64_t> markers16;
    map<Marker32, int64_t> markers32;
    map<int64_t, variant<Offset16, Offset32>> offsets;
    for (auto const &f : fragments) {
      if (holds_alternative<DataFragmentPtr>(f)) {
        auto const &df = get<DataFragmentPtr>(f);
        int &count = usage[df];
        count--;
        if (count == 0) {
          int64_t const pos = upstream->position();
          if (!upstream->write(df->data.data(), df->data.size())) {
            return EGLYF_ERROR;
          }
          for (auto const &marker : df->markers) {
            if (holds_alternative<Marker16>(marker)) {
              auto const &m16 = get<Marker16>(marker);
              auto found = markers16.find(m16);
              if (found == markers16.end()) {
                return EGLYF_ERROR;
              }
              int64_t offset = pos - base;
              if (offset < 0 || offset > numeric_limits<Offset16>::max()) {
                return EGLYF_ERROR;
              }
              Offset16 offset16 = offset;
              offsets[found->second] = offset16;
              markers16.erase(found);
            } else if (holds_alternative<Marker32>(marker)) {
              auto const &m32 = get<Marker32>(marker);
              auto found = markers32.find(m32);
              if (found == markers32.end()) {
                return EGLYF_ERROR;
              }
              int64_t offset = pos - base;
              if (offset < 0 || offset > numeric_limits<Offset32>::max()) {
                return EGLYF_ERROR;
              }
              Offset32 offset32 = offset;
              offsets[found->second] = offset32;
              markers32.erase(found);
            } else {
              return EGLYF_ERROR;
            }
          }
        }
      } else if (holds_alternative<HeaderFragment>(f)) {
        auto const &hf = get<HeaderFragment>(f);
        int64_t beginning = upstream->position();
        if (!upstream->write(hf.data.data(), hf.data.size())) {
          return EGLYF_ERROR;
        }
        for (auto const &[position, marker] : hf.markers) {
          if (holds_alternative<Marker16>(marker)) {
            Marker16 m16 = get<Marker16>(marker);
            markers16[m16] = beginning + position;
          } else if (holds_alternative<Marker32>(marker)) {
            Marker32 m32 = get<Marker32>(marker);
            markers32[m32] = beginning + position;
          } else {
            return EGLYF_ERROR;
          }
        }
      }
    }

    if (!markers16.empty()) {
      return EGLYF_ERROR;
    }
    if (!markers32.empty()) {
      return EGLYF_ERROR;
    }

    int64_t end = upstream->position();
    for (auto [position, offset] : offsets) {
      if (!upstream->seek(position)) {
        return EGLYF_ERROR;
      }
      if (holds_alternative<Offset16>(offset)) {
        if (!upstream->o16(get<Offset16>(offset))) {
          return EGLYF_ERROR;
        }
      } else if (holds_alternative<Offset32>(offset)) {
        if (!upstream->o32(get<Offset32>(offset))) {
          return EGLYF_ERROR;
        }
      } else {
        return EGLYF_ERROR;
      }
    }
    if (!upstream->seek(end)) {
      return EGLYF_ERROR;
    }

    return Status::Ok();
  }

private:
  OutputStream *const upstream;
  int64_t const base;
  std::vector<std::variant<HeaderFragment, std::shared_ptr<DataFragment>>> fragments;
  std::variant<ActiveHeaderFragment, ActiveDataFragment> active;
  std::map<std::string, std::shared_ptr<DataFragment>> lut;
};

} // namespace eglyf

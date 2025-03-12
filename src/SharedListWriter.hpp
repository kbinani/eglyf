#pragma once

namespace eglyf {

template <class T>
  requires requires(T const &t, OutputStream &out) {
    { t.write(out) } -> std::convertible_to<Status>;
    { t.size() } -> std::convertible_to<size_t>;
  }
class SharedListWriter {
public:
  static Optional<SharedListWriter> WriteOffsets16(OutputStream &out, std::shared_ptr<OffsetWriter> const &writer, std::vector<std::shared_ptr<T>> const &list) {
    using namespace std;
    SharedListWriter ret;
    for (auto const &v : list) {
      auto offset = writer->o16();
      if (!offset) {
        return EGLYF_NULLOPT;
      }
      ret.offsets[v].push_back(offset);
    }
    return ret;
  }

  Status writeList(OutputStream &out) {
    using namespace std;
    vector<pair<shared_ptr<T>, vector<OffsetWriter::Handle16>>> ordered;
    for (auto const &it : offsets) {
      ordered.push_back(it);
    }
    ranges::sort(ordered, [](auto const &a, auto const &b) { return a.first->size() < b.first->size(); });
    for (auto [v, offset] : ordered) {
      for (auto it : offset) {
        if (auto st = it->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      if (auto st = v->write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    return Status::Ok();
  }

private:
  std::map<std::shared_ptr<T>, std::vector<OffsetWriter::Handle16>> offsets;
};

} // namespace eglyf

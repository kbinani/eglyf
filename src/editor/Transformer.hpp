#pragma once

namespace eglyf {

class Transformer {
  Transformer() = delete;

public:
  static Status Transform(std::shared_ptr<Font> const &font, Config cfg) {
    using namespace std;
    auto editor = make_shared<Editor>(font, cfg);
    if (auto st = editor->preprocess(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    VtpParser parser(editor);
    if (auto st = parser.parseVtp(res::vtp); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = editor->postprocess(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = editor->compile(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return Status::Ok();
  }
};

} // namespace eglyf
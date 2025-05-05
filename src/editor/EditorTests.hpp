#pragma once

#if EGLYF_ENABLE_TESTS

namespace eglyf::tests {

class EditorTests : public juce::UnitTest {
  using HbBlobUniquePtr = std::unique_ptr<hb_blob_t, juce::FunctionPointerDestructor<hb_blob_destroy>>;
  using HbFaceUniquePtr = std::unique_ptr<hb_face_t, juce::FunctionPointerDestructor<hb_face_destroy>>;
  using HbFontUniquePtr = std::unique_ptr<hb_font_t, juce::FunctionPointerDestructor<hb_font_destroy>>;
  using HbBufferUniquePtr = std::unique_ptr<hb_buffer_t, juce::FunctionPointerDestructor<hb_buffer_destroy>>;

  struct GlyphInformation {
    hb_codepoint_t glyphID;
    hb_position_t x;
    hb_position_t y;
    uint32_t cluster;
  };

  static juce::String JuceStringFromU32String(std::u32string const &s) {
    return juce::String(juce::CharPointer_UTF32((juce::juce_wchar *)s.c_str()), juce::CharPointer_UTF32((juce::juce_wchar *)(s.c_str() + s.size())));
  }

  static juce::String JuceStringFromU8String(std::u8string const &s) {
    return juce::String::fromUTF8((char const *)s.c_str(), s.size());
  }

  static std::u8string U8StringFromJuceString(juce::String const &s) {
    return std::u8string((char8_t const *)s.toRawUTF8());
  }

  static std::u8string U8StringFromU32String(std::u32string const &s) {
    return U8StringFromJuceString(JuceStringFromU32String(s));
  }

  static std::u32string U32StringFromJuceString(juce::String const &s) {
    return std::u32string((char32_t const *)s.toUTF32().getAddress());
  }

  static std::u32string U32StringFromU8string(std::u8string const &s) {
    return U32StringFromJuceString(juce::String::fromUTF8(s.c_str()));
  }

  static std::shared_ptr<hb_font_t> HbMakeSharedFontPtr(hb_font_t *ptr) {
    return std::shared_ptr<hb_font_t>(ptr, hb_font_destroy);
  }

  static hb_buffer_t *CreateBuffer(std::u32string const &t, std::shared_ptr<hb_font_t> const &font) {
    std::u8string utf8 = U8StringFromU32String(t);

    HbBufferUniquePtr buffer(hb_buffer_create());
    hb_buffer_add_utf8(buffer.get(), (char const *)utf8.c_str(), -1, 0, -1);
    hb_buffer_set_direction(buffer.get(), HB_DIRECTION_LTR);
    hb_buffer_set_script(buffer.get(), HB_SCRIPT_EGYPTIAN_HIEROGLYPHS);
    hb_buffer_set_cluster_level(buffer.get(), HB_BUFFER_CLUSTER_LEVEL_CHARACTERS);
    hb_shape(font.get(), buffer.get(), nullptr, 0);

    return buffer.release();
  }

  static void CreateGlyphInformations(HbBufferUniquePtr const &buffer, std::shared_ptr<hb_font_t> const &font, std::vector<GlyphInformation> &out) {
    out.clear();

    hb_font_extents_t extents{};
    hb_font_get_h_extents(font.get(), &extents);
    auto descender = extents.descender;
    int unitsPerEm = hb_face_get_upem(hb_font_get_face(font.get()));

    unsigned int numGlyphs = hb_buffer_get_length(buffer.get());
    hb_glyph_info_t *glyphInfo = hb_buffer_get_glyph_infos(buffer.get(), nullptr);
    hb_glyph_position_t *glyphPos = hb_buffer_get_glyph_positions(buffer.get(), nullptr);
    hb_position_t cursorX = 0;
    hb_position_t cursorY = -(unitsPerEm + descender);
    for (unsigned int i = 0; i < numGlyphs; i++) {
      GlyphInformation info;
      info.glyphID = glyphInfo[i].codepoint;
      info.cluster = glyphInfo[i].cluster;
      auto xOffset = glyphPos[i].x_offset;
      auto yOffset = glyphPos[i].y_offset;
      auto xAdvance = glyphPos[i].x_advance;
      auto yAdvance = glyphPos[i].y_advance;
      info.x = cursorX + xOffset;
      info.y = -(cursorY + yOffset);
      out.push_back(info);
      cursorX += xAdvance;
      cursorY += yAdvance;
    }
  }

  static bool GlyphNames(std::vector<GlyphInformation> const &infos, FontFile const &font, std::vector<std::string> &names) {
    names.clear();
    for (auto const &info : infos) {
      if (auto name = font.post->getName(info.glyphID); name) {
        names.push_back(*name);
      } else {
        return false;
      }
    }
    return true;
  }

  void fail(Status st) {
    using namespace std;
    if (st.ok()) {
      return;
    }
    stringstream out;
    st.print(out);
    expect(false, juce::String(out.str()));
  }

public:
  EditorTests(juce::File sourceFontFile, juce::File referenceFontFile) : juce::UnitTest(""), sourceFontFile(sourceFontFile), referenceFontFile(referenceFontFile) {}

  void runTest() override {
    using namespace std;

    beginTest("prepare");

    string_view vtp(BinaryData::EgyptianText_200_vtp, BinaryData::EgyptianText_200_vtpSize);

    shared_ptr<FontFile> font;
    HbBlobUniquePtr hbBlob;
    HbFaceUniquePtr hbFace;
    shared_ptr<hb_font_t> hbFont;
    string data;
    {
      FileInputStream fis(sourceFontFile);
      if (auto st = FontFile::Read(fis, font); !st.ok()) {
        fail(st);
        return;
      }
      Config cfg;
      auto editor = make_shared<Editor>(font, cfg);
      if (auto st = editor->preprocess(); !st.ok()) {
        fail(st);
        return;
      }
      VtpParser parser(editor);
      if (auto st = parser.parseVtp(vtp); !st.ok()) {
        fail(st);
        return;
      }
      if (auto st = editor->postprocess(); !st.ok()) {
        fail(st);
        return;
      }
      if (auto st = editor->compile(); !st.ok()) {
        fail(st);
        return;
      }
      ByteOutputStream out;
      if (auto st = font->write(out); !st.ok()) {
        fail(st);
        return;
      }
      data = out.data();
      hbBlob.reset(hb_blob_create(data.data(),
                                  data.size(),
                                  HB_MEMORY_MODE_READONLY,
                                  nullptr,
                                  nullptr));
      hbFace.reset(hb_face_create(hbBlob.get(), 0));
      hbFont = HbMakeSharedFontPtr(hb_font_create(hbFace.get()));
    }

    shared_ptr<FontFile> refFont;
    HbBlobUniquePtr hbRefBlob;
    HbFaceUniquePtr hbRefFace;
    shared_ptr<hb_font_t> hbRefFont;
    string refData;
    {
      FileInputStream fis(referenceFontFile);
      if (auto st = FontFile::Read(fis, refFont); !st.ok()) {
        fail(st);
        return;
      }
      //      auto editor = make_shared<Editor>(refFont);
      //      VtpParser parser(editor);
      //      if (auto st = parser.parseVtp(vtp); !st.ok()) {
      //        fail(st);
      //        return;
      //      }
      //      if (auto st = editor->compile(); !st.ok()) {
      //        fail(st);
      //        return;
      //      }
      ByteOutputStream out;
      if (auto st = refFont->write(out); !st.ok()) {
        fail(st);
        return;
      }
      refData = out.data();
      hbRefBlob.reset(hb_blob_create(refData.data(),
                                     refData.size(),
                                     HB_MEMORY_MODE_READONLY,
                                     nullptr,
                                     nullptr));
      hbRefFace.reset(hb_face_create(hbRefBlob.get(), 0));
      hbRefFont = HbMakeSharedFontPtr(hb_font_create(hbRefFace.get()));
    }

#include "Glyphs.hpp"

    u32string sentence = A1 + hj + B1 + vj + Z2;
    HbBufferUniquePtr buf(CreateBuffer(sentence, hbFont));
    vector<GlyphInformation> infos;
    CreateGlyphInformations(buf, hbFont, infos);
    vector<string> names;
    GlyphNames(infos, *font, names);

    HbBufferUniquePtr refBuf(CreateBuffer(sentence, hbRefFont));
    vector<GlyphInformation> refInfos;
    CreateGlyphInformations(refBuf, hbRefFont, refInfos);
    vector<string> refNames;
    GlyphNames(refInfos, *refFont, refNames);

    cout << "actual:" << endl;
    for (size_t j = 0; j < names.size(); j++) {
      auto const &n = names[j];
      cout << "  [" << j << "] " << n << endl;
    }

    cout << "expected:" << endl;
    for (size_t j = 0; j < refNames.size(); j++) {
      auto const &n = refNames[j];
      cout << "  [" << j << "] " << n << endl;
    }

    cout << "actual = " << names.size() << ", expected = " << refNames.size() << endl;
    expect(names.size() == refNames.size());

    for (int i = 0; i < refNames.size() - 1; i++) {
      expect(names[i] == refNames[i]);
    }
  }

private:
  juce::File sourceFontFile;
  juce::File referenceFontFile;
};

} // namespace eglyf::tests

#endif

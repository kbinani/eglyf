#pragma once

#if EGLYF_ENABLE_TESTS

namespace eglyf::tests {

class EditorTests : public juce::UnitTest {
  using HbBlobUniquePtr = std::unique_ptr<hb_blob_t, juce::FunctionPointerDestructor<hb_blob_destroy>>;
  using HbFaceUniquePtr = std::unique_ptr<hb_face_t, juce::FunctionPointerDestructor<hb_face_destroy>>;
  using HbFontUniquePtr = std::unique_ptr<hb_font_t, juce::FunctionPointerDestructor<hb_font_destroy>>;
  using HbBufferUniquePtr = std::unique_ptr<hb_buffer_t, juce::FunctionPointerDestructor<hb_buffer_destroy>>;

  struct GlyphInformation {
    hb_codepoint_t glyphId;
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
      info.glyphId = glyphInfo[i].codepoint;
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
      if (auto name = font.post->getName(info.glyphId); name) {
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

  void initialise() override {
    using namespace std;
    FileInputStream fis(sourceFontFile);
    if (auto st = FontFile::Read(fis, font); !st.ok()) {
      fail(st);
      return;
    }
    auto editor = make_shared<Editor>(font);
    VtpParser parser(editor);
    string_view vtp(BinaryData::EgyptianText_200_vtp, BinaryData::EgyptianText_200_vtpSize);
    if (auto st = parser.parseVtp(vtp); !st.ok()) {
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
    string data = out.data();

    hbBlob.reset(hb_blob_create(data.data(),
                                data.size(),
                                HB_MEMORY_MODE_READONLY,
                                nullptr,
                                nullptr));
    hbFace.reset(hb_face_create(hbBlob.get(), 0));
    hbFont = HbMakeSharedFontPtr(hb_font_create(hbFace.get()));

    FileInputStream refStream(referenceFontFile);
    string refData = refStream.readUntilEos();
    ByteInputStream input(refData);
    if (auto st = FontFile::Read(input, ref); !st.ok()) {
      fail(st);
      return;
    }
    ByteOutputStream tout;
    if (auto st = ref->write(tout); !st.ok()) {
      fail(st);
      return;
    }
    auto rewrite = tout.data();
    hbRefBlob.reset(hb_blob_create(rewrite.data(),
                                   rewrite.size(),
                                   HB_MEMORY_MODE_READONLY,
                                   nullptr,
                                   nullptr));
    hbRefFace.reset(hb_face_create(hbRefBlob.get(), 0));
    hbRefFont = HbMakeSharedFontPtr(hb_font_create(hbRefFace.get()));
  }

  void runTest() override {
    using namespace std;

    auto const vj = U"\U00013430"s;
    auto const p = U"𓊪"s;
    auto const n = U"𓈖"s;
    auto const cb = U"\U00013379"s;
    auto const esb = U"\U00013220"s;
    auto const A1 = U"\U00013000"s;
    auto const B1 = U"\U00013050"s;
    auto const O1 = U"\U00013250"s;
    auto const C1 = U"\U0001305a"s;
    auto const ese = U"\U00013221"s;
    auto const ce = U"\U0001337a"s;
    auto const cwb = U"\U00013286"s;
    auto const ewb = U"\U00013222"s;
    auto const ewe = U"\U00013223"s;
    auto const cwe = U"\U00013287"s;
    auto const M17 = U"\U000131cb"s;
    auto const Y5 = U"\U000133e0"s;
    auto const N35 = U"\U00013216"s;
    auto const R4 = U"\U000132b5"s;
    auto const X1 = U"\U000133cf"s;
    auto const hj = U"\U00013431"s;
    auto const Q3 = U"\U000132aa"s;
    auto const hwtb = U"\U00013258"s;
    auto const hwtbe = U"\U0001325c"s;
    auto const R8 = U"\U000132b9"s;
    auto const G7 = U"\U00013146"s;
    auto const G25 = U"\U0001315c"s;
    auto const te = U"\U00013434"s;
    auto const J1 = U"\U0001340d"s;
    auto const hwte = U"\U0001325d"s;
    auto const hwtte = U"\U0001325b"s;
    auto const hwtbb = U"\U00013259"s;
    auto const Z2 = U"\U000133e5"s;
    auto const hwttb = U"\U0001325a"s;
    auto const D17 = U"\U00013087"s;
    auto const D36 = U"\U0001309d"s;
    auto const om = U"\U00013436"s;
    auto const V28 = U"\U0001339b"s;
    auto const W24 = U"\U000133cc"s;
    auto const Z7 = U"\U000133f2"s;
    auto const F4 = U"\U00013102"s;
    auto const ts = U"\U00013432"s;
    auto const G39 = U"\U0001316d"s;
    auto const bs = U"\U00013433"s;
    auto const I10 = U"\U00013193"s;
    auto const D46 = U"\U000130a7"s;
    auto const be = U"\U00013435"s;
    auto const M2 = U"\U000131d0"s;
    auto const S34 = U"\U00013319"s;
    auto const T22 = U"\U00013342"s;
    auto const H6 = U"\U000131d4"s;
    auto const J12 = U"\U00013442"s;
    auto const Q1 = U"\U000132b8"s;
    auto const N34 = U"\U0001320c"s;
    auto const S29 = U"\U00013314"s;
    auto const N25 = U"\U00013209"s;
    auto const F34 = U"\U00013173"s;
    auto const G43 = U"\U000131c1"s;
    auto const G43a = U"\U000131c2"s;
    auto const I11a = U"\U000131e6"s;
    auto const L2 = U"\U000131f4"s;
    auto const L2a = U"\U000131f5"s;
    auto const O30 = U"\U000132cd"s;
    auto const O30a = U"\U000132ce"s;
    auto const P8 = U"\U000132b4"s;
    auto const P9 = U"\U000132b5"s;
    auto const W14 = U"\U00013407"s;
    auto const W14a = U"\U00013408"s;
    auto const U7 = U"\U0001333b"s;
    auto const M3 = U"\U000131d1"s;
    auto const Z1 = U"\U000133e4"s;
    auto const D2 = U"\U00013077"s;
    auto const N1 = U"\U000131ef"s;
    auto const D4 = U"\U00013079"s;
    auto const M8 = U"\U000131d7"s;
    auto const G1 = U"\U0001313f"s;
    auto const G36 = U"\U00013168"s;
    auto const F20 = U"\U00013113"s;
    auto const G9 = U"\U0001314a"s;
    auto const N27 = U"\U0001320c"s;
    auto const N18 = U"\U00013200"s;
    auto const S30 = U"\U00013315"s;
    auto const G38 = U"\U0001316c"s;
    auto const D21 = U"\U0001308b"s;
    auto const N28 = U"\U0001320d"s;
    auto const Y1 = U"\U000133db"s;
    auto const V1 = U"\U00013362"s;
    auto const N5 = U"\U000131f3"s;
    auto const ss = U"\U00013437"s;
    auto const se = U"\U00013438"s;
    auto const A15 = U"\U00013012"s;
    auto const Z4 = U"\U000133ed"s;
    auto const N23 = U"\U00013207"s;
    auto const U1 = U"\U00013333"s;
    auto const F51 = U"\U00013139"s;
    auto const I9 = U"\U00013191"s;
    auto const V30 = U"\U0001339f"s;
    auto const N17 = U"\U000131ff"s;
    auto const N37 = U"\U00013219"s;
    auto const R7 = U"\U000132b8"s;
    auto const Z9 = U"\U000133f4"s;
    auto const D40 = U"\U000130a1"s;
    auto const T10 = U"\U00013314"s;
    auto const D7 = U"\U0001307c"s;
    auto const N33 = U"\U00013212"s;
    auto const M40 = U"\U000131e9"s;
    auto const N16 = U"\U000131fe"s;
    auto const N21 = U"\U00013205"s;
    auto const M23 = U"\U000131d3"s;
    auto const J15 = U"\U0001341d"s;
    auto const Z11 = U"\U000133f6"s;
    auto const F35 = U"\U00013124"s;
    auto const Z4a = U"\U000133ee"s;
    auto const O34 = U"\U00013283"s;

    vector<u32string> sentences;
    sentences.push_back(p);
    sentences.push_back(p + vj + n);
    sentences.push_back(U"𓇋𓅱𓏞𓏜𓀀𓂋𓐰𓏤𓈖𓆎𓅓𓏏𓐰𓊖"s);
    sentences.push_back(U"𓍹𓐼𓇋𓏠𓐰𓈖𓅱𓐳𓏏𓐴𓏏𓋹𓐽𓍺"s);
    sentences.push_back(U"𓆓𓐳𓐷𓂧𓐰𓂧𓐸𓐴𓏏𓅓𓍛𓏤𓐰𓈖𓍉𓐰𓎂𓉐𓋹𓍑𓋴"s);
    sentences.push_back(U"𓍹𓐼𓁩𓑀𓁛𓈘𓐰𓄟𓐱𓋴𓐱𓋴𓐽𓍺"s);
    sentences.push_back(U"𓄿𓑇𓄿𓑊𓄿𓑈𓄿𓑎"s);
    sentences.push_back(cb + esb + A1 + B1 + vj + O1 + C1 + ese + ce);
    sentences.push_back(esb + A1 + B1 + C1 + ese + ce);
    sentences.push_back(cb + A1 + B1 + C1 + ce);
    sentences.push_back(cb + esb + A1 + B1 + C1 + ese);
    sentences.push_back(cb + esb + A1 + cb + esb + B1 + ese + ce + C1 + ese + ce);
    sentences.push_back(cb + esb + A1 + esb + B1 + ese + ce + C1 + ese + ce);
    sentences.push_back(cb + esb + A1 + cb + B1 + ce + C1 + ese + ce);
    sentences.push_back(cb + esb + A1 + cb + esb + B1 + ese + C1 + ese + ce);
    sentences.push_back(esb + A1 + cb + esb + B1 + ese + ce + C1 + ese + ce);
    sentences.push_back(cb + A1 + cb + esb + B1 + ese + ce + C1 + ce);
    sentences.push_back(cwb + ewb + A1 + B1 + vj + O1 + C1 + ewe + cwe);
    sentences.push_back(cb + esb + cwb + ewb + A1 + B1 + vj + O1 + C1 + ewe + cwe + ese + ce);
    sentences.push_back(cb + M17 + Y5 + vj + N35 + R4 + vj + X1 + hj + Q3 + ce);
    sentences.push_back(hwtb + R8 + hwtbe);
    sentences.push_back(hwtb + X1 + vj + Q3 + hwtte);
    sentences.push_back(hwttb + G7 + hwte);
    sentences.push_back(hwtbb + G25 + te + J1 + hwte);
    sentences.push_back(cwb + A1 + hj + B1 + vj + Z2 + cwe);
    sentences.push_back(A1);
    sentences.push_back(A1 + vj + O1);
    sentences.push_back(W24 + hj + Z7);
    sentences.push_back(F4 + ts + X1);
    sentences.push_back(G39 + bs + X1);
    sentences.push_back(I10 + bs + D46);
    sentences.push_back(D17 + te + X1);
    sentences.push_back(G25 + be + X1);
    sentences.push_back(D36 + om + V28);
    sentences.push_back(N35 + vj + X1);
    sentences.push_back(D2 + vj + D21 + vj + N1);
    sentences.push_back(N28 + vj + D36 + vj + D36 + vj + Y1);
    sentences.push_back(Z1 + vj + Z1 + vj + Z1 + vj + Z1 + vj + Z1);
    sentences.push_back(X1 + vj + X1 + vj + X1 + vj + X1 + vj + X1 + vj + X1);
    sentences.push_back(N35 + vj + N35 + vj + N35 + vj + N35 + vj + N35 + vj + N35);
    sentences.push_back(V1 + hj + V1 + hj + V1);
    sentences.push_back(Z1 + hj + Z1 + hj + Z1 + hj + Z1 + hj + Z1 + hj + Z1);
    sentences.push_back(R4 + vj + X1 + hj + Q3);
    sentences.push_back(D2 + hj + J1 + vj + N35);
    sentences.push_back(M17 + hj + ss + D2 + vj + N5 + se);
    sentences.push_back(ss + D2 + vj + N5 + se + hj + M17);
    sentences.push_back(X1 + hj + X1 + vj + N35);
    sentences.push_back(A15 + vj + N23 + hj + Z1);
    sentences.push_back(A1 + hj + B1 + vj + Z2);
    sentences.push_back(D21 + vj + D21 + vj + Z7 + hj + Z4);
    sentences.push_back(F4 + vj + X1 + hj + X1 + vj + D36);
    sentences.push_back(D21 + vj + M17 + hj + X1 + hj + M17);
    sentences.push_back(J1 + hj + X1 + vj + Y1 + vj + Z2);
    sentences.push_back(D36 + hj + U1 + vj + X1 + hj + F51);
    sentences.push_back(D2 + hj + Z1 + hj + G7 + vj + N35);
    sentences.push_back(D21 + vj + F4 + vj + X1 + hj + Z1 + vj + I9);
    sentences.push_back(V30 + vj + N17 + vj + N17 + vj + N23 + hj + N23);
    sentences.push_back(N37 + vj + R7 + hj + R7 + vj + Z9 + vj + D40);
    sentences.push_back(T10 + vj + X1 + hj + Z2 + vj + Z2 + hj + Z2);
    sentences.push_back(D21 + vj + X1 + hj + Q3 + hj + X1 + vj + D36);
    sentences.push_back(D7 + vj + N33 + hj + N33 + hj + N33 + hj + N33);
    sentences.push_back(N35 + vj + M40 + hj + M40 + hj + M40 + hj + M40 + hj + M40 + hj + M40 + vj + N35);
    sentences.push_back(D36 + hj + Z4 + vj + Z7 + hj + D21 + hj + Z1);
    sentences.push_back(N16 + vj + N16 + vj + N16 + vj + N21 + hj + N21 + hj + N21);
    sentences.push_back(N35 + vj + X1 + vj + X1 + vj + X1 + vj + X1);
    sentences.push_back(N35 + vj + X1 + hj + X1 + hj + X1 + hj + X1);
    sentences.push_back(D2 + hj + X1 + hj + X1 + vj + X1 + hj + D2 + hj + X1 + vj + X1 + hj + X1 + hj + D2);
    sentences.push_back(M23 + hj + ss + X1 + vj + N35 + se);
    sentences.push_back(J15 + vj + Z11 + hj + ss + D2 + vj + D21 + se);
    sentences.push_back(F35 + hj + ss + Z1 + vj + Z4a + se);
    sentences.push_back(O34 + vj + V28 + hj + ss + M2 + vj + Z7 + se);
    sentences.push_back(V28 + hj + ss + D36 + vj + Z4a + se + vj + J15);
    sentences.push_back(M17 + hj + ss + X1 + vj + N35 + vj + N5 + se);
    sentences.push_back(M17 + hj + ss + X1 + vj + N35 + vj + N5 + se + hj + S34);
    sentences.push_back(T22 + hj + ss + N35 + vj + N34 + vj + J12 + vj + N35 + se);
    sentences.push_back(H6 + hj + ss + D21 + vj + X1 + se);
    sentences.push_back(N35 + vj + A1 + hj + ss + X1 + vj + X1 + se + vj + N35);
    sentences.push_back(N35 + vj + Q1 + hj + ss + X1 + vj + F34 + hj + Z1 + se + vj + I9);
    sentences.push_back(Z11 + hj + ss + D2 + hj + D21 + vj + N25 + se);
    sentences.push_back(ss + M8 + vj + G1 + se + hj + M40);
    sentences.push_back(N35 + vj + ss + W24 + vj + X1 + se + hj + Z1 + hj + Z1);
    sentences.push_back(X1 + hj + N35 + vj + ss + Y1 + vj + Z2 + se + hj + S29);
    sentences.push_back(ss + Y1 + vj + Z2 + se + hj + S29 + vj + X1 + hj + N35);
    sentences.push_back(ss + A1 + vj + B1 + se + hj + ss + B1 + vj + A1 + se);
    sentences.push_back(Z11 + hj + ss + D2 + hj + ss + D21 + vj + X1 + se + vj + N25 + se);
    sentences.push_back(N35 + vj + A1 + hj + ss + X1 + vj + Z1 + hj + ss + Z1 + vj + D21 + se + se);
    sentences.push_back(J15 + vj + Z11 + hj + ss + D2 + hj + ss + D21 + vj + X1 + se + vj + N25 + se);
    sentences.push_back(ss + I10 + bs + D46 + vj + I9 + se + hj + M17);
    sentences.push_back(I10 + bs + ss + ss + X1 + vj + D21 + se + hj + D2 + se);
    sentences.push_back(ss + D2 + hj + ss + D21 + vj + X1 + se + vj + N25 + se + hj + Z11 + vj + N35);
    sentences.push_back(ss + ss + D21 + vj + X1 + se + hj + D2 + vj + N25 + se + hj + Z11 + vj + N35);
    sentences.push_back(N35 + vj + F4 + ts + X1);
    sentences.push_back(F4 + ts + X1 + vj + W24 + vj + Z2);
    sentences.push_back(G25 + bs + J1);
    sentences.push_back(G36 + bs + X1 + vj + N35);
    sentences.push_back(D36 + vj + I10 + bs + D46);
    sentences.push_back(I9 + vj + N35 + vj + F20 + bs + A1);
    sentences.push_back(I10 + bs + ss + I10 + bs + I10 + se);
    sentences.push_back(G9 + te + N27);
    sentences.push_back(D17 + te + X1 + vj + N37);
    sentences.push_back(G9 + te + ss + N27 + vj + N27 + se);
    sentences.push_back(N35 + vj + H6 + hj + ss + G39 + be + X1 + vj + D21 + se);
    sentences.push_back(G36 + bs + D21 + te + X1);
    sentences.push_back(G25 + bs + D21 + te + X1);
    sentences.push_back(G25 + te + J1 + be + X1);
    sentences.push_back(G25 + bs + D21 + te + N5 + be + X1);
    sentences.push_back(N35 + vj + I10 + bs + ss + X1 + vj + Z1 + se);
    sentences.push_back(S34 + hj + I10 + bs + ss + X1 + vj + J12 + se);
    sentences.push_back(I10 + bs + ss + D46 + vj + I9 + se + hj + M17);
    sentences.push_back(G9 + bs + S34 + te + ss + N27 + vj + N27 + se);
    sentences.push_back(G39 + bs + X1 + te + ss + X1 + hj + Z1 + se + be + X1);
    sentences.push_back(N35 + vj + G9 + te + X1 + hj + ss + D2 + vj + D21 + se);
    sentences.push_back(I10 + bs + ss + D2 + hj + ss + X1 + vj + D21 + se + se);
    sentences.push_back(F20 + bs + ss + Z11 + hj + ss + X1 + vj + D21 + se + vj + N35 + se);
    sentences.push_back(F20 + bs + ss + ss + J1 + vj + X1 + se + hj + Z11 + se + vj + N35);
    sentences.push_back(F20 + bs + ss + ss + J1 + vj + X1 + se + hj + D2 + vj + N35 + se);
    sentences.push_back(ss + D36 + vj + D36 + se + om + V28);
    sentences.push_back(D36 + om + ss + V28 + hj + V28 + se);
    sentences.push_back(ss + D36 + vj + D36 + se + om + ss + V28 + hj + V28 + se);
    sentences.push_back(D36 + om + V28 + vj + N35);
    sentences.push_back(ss + I10 + bs + D46 + se + om + V28);
    sentences.push_back(Z1 + hj + Z1 + hj + Z1 + hj + Z1 + hj + Z1 + hj + Z1 + vj + X1 + vj + Z1 + hj + Z1 + hj + Z1 + hj + Z1 + hj + Z1 + hj + Z1 + vj + X1 + vj + Z1 + hj + Z1 + hj + Z1);
    sentences.push_back(A1 + se);
    sentences.push_back(G9 + te + ss + N27 + vj + N27 + se + bs + S34);
    sentences.push_back(ss + ss + D21 + vj + X1 + se + se);
    sentences.push_back(ss + ss + D21 + vj + Z1 + hj + A1 + se + se);
    sentences.push_back(ss + D2 + hj + X1 + vj + N25 + se);
    sentences.push_back(D36 + om + ss + I10 + bs + X1 + se);
    sentences.push_back(G43a + G43 + bs + X1);
    sentences.push_back(I11a + I10 + bs + ss + X1 + vj + N18 + se);
    sentences.push_back(L2a + M23 + hj + L2 + vj + X1 + hj + X1);
    sentences.push_back(O30a + O30 + hj + O30 + hj + O30 + hj + O30);
    sentences.push_back(P9 + P8 + om + I9);
    sentences.push_back(S30 + S29 + om + I9);
    sentences.push_back(W14a + V28 + hj + W14 + vj + O34);
    sentences.push_back(D4 + vj + D4 + M23 + hj + ss + X1 + vj + N35 + se + J15 + vj + Q1 + hj + ss + X1 + vj + O1 + se + V30);
    sentences.push_back(D4 + vj + Q1 + I10 + bs + ss + V28 + hj + G43 + se + X1 + vj + N35 + vj + M3 + vj + J1 + hj + X1);
    sentences.push_back(G38 + vj + I9 + U7 + hj + M17 + hj + M17 + vj + I9 + N35 + vj + Q1 + hj + ss + X1 + vj + F34 + hj + Z1 + se + vj + I9);
    sentences.push_back(M17 + hj + ss + X1 + vj + D21 + A1 + se);

    for (size_t i = 0; i < sentences.size(); i++) {
      u32string sentence = sentences[i];
      beginTest(juce::String("[" + to_string(i) + "] ") + JuceStringFromU32String(sentence));

      HbBufferUniquePtr buf(CreateBuffer(sentence, hbFont));
      vector<GlyphInformation> infos;
      CreateGlyphInformations(buf, hbFont, infos);
      vector<string> names;
      GlyphNames(infos, *font, names);
      cout << "actual:" << endl;
      for (auto const &n : names) {
        cout << n << endl;
      }

      HbBufferUniquePtr refBuf(CreateBuffer(sentence, hbRefFont));
      vector<GlyphInformation> refInfos;
      CreateGlyphInformations(refBuf, hbRefFont, refInfos);
      vector<string> refNames;
      GlyphNames(refInfos, *ref, refNames);
      cout << "expected:" << endl;
      for (auto const &n : refNames) {
        cout << n << endl;
      }

      expect(names.size() == refNames.size());
      for (size_t i = 0; i < names.size(); i++) {
        expect(names[i] == refNames[i]);
      }
    }
  }

private:
  juce::File sourceFontFile;
  juce::File referenceFontFile;

  std::shared_ptr<FontFile> font;
  HbBlobUniquePtr hbBlob;
  HbFaceUniquePtr hbFace;
  std::shared_ptr<hb_font_t> hbFont;

  std::shared_ptr<FontFile> ref;
  HbBlobUniquePtr hbRefBlob;
  HbFaceUniquePtr hbRefFace;
  std::shared_ptr<hb_font_t> hbRefFont;
};

} // namespace eglyf::tests

#endif

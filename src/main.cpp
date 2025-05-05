#define EGLYF_ENABLE_TESTS 0

#include "eglyf.hpp"

#if EGLYF_ENABLE_TESTS
#include "editor/EditorTests.hpp"
#include "gfx/QuadraticBezierTests.hpp"
#endif

#include <juce_core/juce_core.h>

static void Fail(eglyf::Status st) {
  std::stringstream out;
  st.print(out);
  juce::ConsoleApplication::fail(juce::String(out.str()));
}

static std::u16string U16StringFromJuceString(juce::String const &s) {
  return std::u16string((char16_t const *)s.toUTF16().getAddress());
}

#if EGLYF_ENABLE_TESTS
static void Test(juce::ArgumentList const &args) {
  using namespace eglyf::tests;

  //  juce::File input = args.getExistingFileForOption("--input");
  //  juce::File reference = args.getExistingFileForOption("--reference");
  //  EditorTests editorTest(input, reference);

  QuadraticBezierTests quadraticBezierTests;

  juce::UnitTestRunner runner;
  runner.runAllTests();
}
#endif

static void Run(juce::ArgumentList const &args) {
  using namespace std;
  using namespace eglyf;

  Config cfg;

  juce::File input = args.getExistingFileForOption("--input");
  juce::File output = args.getFileForOption("--output");

  juce::String name = args.getValueForOption("--name");
  if (name.length() > 0) {
    auto tokens = juce::StringArray::fromTokens(name, "/", "\"");
    if (tokens.size() != 4) {
      cerr << "--name family/subFamily/fullName/psName" << endl;
      cerr << "ex: --name \"My EgyptHiero/Regular/My Egyptian Hieroglyphs Regular/MyEgyptianHieroglyphs-Regular\"" << endl;
      return;
    }
    Config::Name name;
    name.family = U16StringFromJuceString(tokens[0]);
    name.subFamily = U16StringFromJuceString(tokens[1]);
    name.fullName = U16StringFromJuceString(tokens[2]);
    name.psName = U16StringFromJuceString(tokens[3]);
    cfg.name = name;
  }

  FileInputStream fis(input);
  shared_ptr<FontFile> ff;
  if (auto st = FontFile::Read(fis, ff); !st.ok()) {
    Fail(st);
    return;
  }
  auto editor = make_shared<Editor>(ff, cfg);
  if (auto st = editor->preprocess(); !st.ok()) {
    Fail(st);
    return;
  }

  VtpParser parser(editor);
  string_view vtp(BinaryData::EgyptianText_200_vtp, BinaryData::EgyptianText_200_vtpSize);
  if (auto st = parser.parseVtp(vtp); !st.ok()) {
    Fail(st);
    return;
  }
  if (auto st = editor->postprocess(); !st.ok()) {
    Fail(st);
    return;
  }
  if (auto st = editor->compile(); !st.ok()) {
    Fail(st);
    return;
  }
  FileOutputStream fos(output);
  if (auto st = ff->write(fos); !st.ok()) {
    Fail(st);
  }
}

int main(int argc, char *argv[]) {
  juce::ConsoleApplication app;
  app.addDefaultCommand({juce::String("--run"),
                         juce::String("--run filename"),
                         juce::String(""),
                         juce::String(""),
                         [](auto const &args) { Run(args); }});
#if EGLYF_ENABLE_TESTS
  app.addCommand({juce::String("--test"),
                  "",
                  "",
                  "",
                  [](auto const &args) { Test(args); }});
#endif
  return app.findAndRunCommand(argc, argv);
}

#include "eglyf.hpp"

#include <filesystem>

int main(int argc, char *argv[]) {
  using namespace eglyf;
  using namespace std;
  namespace fs = std::filesystem;

  fs::path input = fs::absolute(fs::path(argv[1]));
  fs::path output = fs::absolute(fs::path(argv[2]));

  FileInputStream fis(juce::File(input.string()));
  shared_ptr<FontFile> ff;
  auto st = FontFile::Read(fis, ff);
  if (!ff) {
    st.print(cout);
    return 255;
  }
  FileOutputStream fos(juce::File(output.string()));
  if (auto st = ff->write(fos); st.ok()) {
    return 0;
  } else {
    st.print(cout);
    return 255;
  }
}

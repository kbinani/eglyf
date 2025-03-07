#include "eglyf.hpp"

#include <filesystem>

int main(int argc, char *argv[]) {
  using namespace eglyf;
  using namespace std;
  namespace fs = std::filesystem;

  fs::path input = fs::absolute(fs::path(argv[1]));
  fs::path output = fs::absolute(fs::path(argv[2]));

  FileInputStream fis(juce::File(input.string()));
  auto ff = FontFile::Read(fis);
  if (!ff) {
    return 1;
  }
  FileOutputStream fos(juce::File(output.string()));
  if (ff->write(fos)) {
    return 0;
  } else {
    return 1;
  }
}

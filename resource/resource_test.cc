#include "resource/resource.h"
#include "dvc/program.h"

int main() {
  dvc::program program;
  std::filesystem::path test_tmpdir = std::getenv("TEST_TMPDIR");

  DVC_ASSERT(is_directory(test_tmpdir));
  DVC_ASSERT(is_empty(test_tmpdir));

  std::filesystem::path indir = test_tmpdir / "indir";

  create_directory(indir);
  dvc::save_file(indir / "fruit", "bananas");
  create_directory(indir / "muppets");
  dvc::save_file(indir / "muppets" / "kermit", "the frog");
  dvc::save_file(indir / "muppets" / "gonzo", "the clown");

  std::filesystem::path outfile = test_tmpdir / "outfile.res";

  ResourceWriter(indir, outfile);

  ResourceReader reader(outfile);

  reader.dump();

  DVC_ASSERT_EQ(reader.get_file("fruit"), "bananas");
  DVC_ASSERT_EQ(reader.get_file("muppets/kermit"), "the frog");
  DVC_ASSERT_EQ(reader.get_file("muppets/gonzo"), "the clown");
}

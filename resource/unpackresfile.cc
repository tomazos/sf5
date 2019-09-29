#include "dvc/opts.h"
#include "dvc/program.h"
#include "resource/resource.h"

std::filesystem::path DVC_OPTION(infile, i, dvc::required,
                                 "input resource file");
std::filesystem::path DVC_OPTION(outdir, o, dvc::required, "output directory");

int main(int argc, char** argv) {
  dvc::program program(argc, argv);
  ResourceReader(infile).unpack(outdir);
}

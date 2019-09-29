#include "dvc/opts.h"
#include "dvc/program.h"
#include "resource/resource.h"

std::filesystem::path DVC_OPTION(indir, i, dvc::required, "input directory");
std::filesystem::path DVC_OPTION(outfile, o, dvc::required,
                                 "output resource file");

int main(int argc, char** argv) {
  dvc::program program(argc, argv);
  ResourceWriter(indir, outfile);
}

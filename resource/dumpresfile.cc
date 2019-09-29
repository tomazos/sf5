#include "dvc/opts.h"
#include "dvc/program.h"
#include "resource/resource.h"

int main(int argc, char** argv) {
  dvc::program program(argc, argv);
  if (dvc::args.size() != 1) DVC_FAIL("Usage: dumpresfile <file.res>");

  ResourceReader(dvc::args.at(0)).dump();
}

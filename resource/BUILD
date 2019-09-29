package(default_visibility = ["//visibility:public"])

cc_library(
    name = "resource",
    srcs = [
        "resource.cc",
    ],
    hdrs = [
        "resource.h",
    ],
    linkopts = [
        "-lboost_iostreams",
    ],
    deps = [
        "//dvc:file",
        "//dvc:log",
    ],
)

cc_test(
    name = "resource_test",
    srcs = [
        "resource_test.cc",
    ],
    deps = [
        ":resource",
        "//dvc:program",
    ],
)

cc_binary(
    name = "genresfile",
    srcs = [
        "genresfile.cc",
    ],
    deps = [
        ":resource",
        "//dvc:program",
    ],
)

cc_binary(
    name = "dumpresfile",
    srcs = [
        "dumpresfile.cc",
    ],
    deps = [
        ":resource",
        "//dvc:program",
    ],
)

cc_binary(
    name = "unpackresfile",
    srcs = [
        "unpackresfile.cc",
    ],
    deps = [
        ":resource",
        "//dvc:program",
    ],
)

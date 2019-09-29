package(default_visibility = ["//visibility:public"])

cc_library(
    name = "rna",
    hdrs = [
        "arena.h",
        "geometry.h",
        "object.h",
        "primitives.h",
    ],
    deps = [
        "//dvc:container",
    ],
)

cc_test(
    name = "rna_test",
    srcs = [
        "rna_test.cc",
    ],
    deps = [
        ":rna",
        "//dvc:log",
    ],
)

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "generate_stars",
    srcs = [
        "generate_stars.cc",
    ],
    linkopts = [
        "-lpng",
    ],
    deps = [
        "//dvc:program",
    ],
)

cc_binary(
    name = "tracer",
    srcs = [
        "tracer.cc",
    ],
    linkopts = [
        "-lpng",
    ],
    deps = [
        "//dvc:file",
        "//dvc:program",
        "//dvc:python",
    ],
)

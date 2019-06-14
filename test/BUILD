load("//:glsl.bzl", "glsl_shader")

glsl_shader(
    name = "triangletest1_vert",
    src = "triangletest1.vert",
)

glsl_shader(
    name = "triangletest1_frag",
    src = "triangletest1.frag",
)

glsl_shader(
    name = "pointtest_vert",
    src = "pointtest.vert",
)

glsl_shader(
    name = "pointtest_frag",
    src = "pointtest.frag",
)

glsl_shader(
    name = "skyfly_vert",
    src = "skyfly.vert",
)

glsl_shader(
    name = "skyfly_frag",
    src = "skyfly.frag",
)

glsl_shader(
    name = "stars_vert",
    src = "stars.vert",
)

glsl_shader(
    name = "stars_frag",
    src = "stars.frag",
)

cc_binary(
    name = "skyfly",
    srcs = [
        "skyfly.cc",
    ],
    data = [
        "face0.png",
        "face1.png",
        "face2.png",
        "face3.png",
        "face4.png",
        "face5.png",
        "skyfly.frag.spv",
        "skyfly.vert.spv",
        "stars.frag.spv",
        "stars.vert.spv",
    ],
    linkopts = [
        "-lgflags",
        "-lpng",
    ],
    deps = [
        "//dvc:file",
        "//dvc:opts",
        "//dvc:terminate",
        "//spk:spkx",
        "//spk:spock",
    ],
)

cc_binary(
    name = "pointtest",
    srcs = [
        "pointtest.cc",
    ],
    data = [
        "pointtest.frag.spv",
        "pointtest.vert.spv",
    ],
    linkopts = [
        "-lgflags",
    ],
    deps = [
        "//dvc:file",
        "//dvc:opts",
        "//dvc:terminate",
        "//spk:spkx",
        "//spk:spock",
    ],
)

cc_binary(
    name = "triangletest1",
    srcs = [
        "triangletest1.cc",
    ],
    data = [
        "triangletest1.frag.spv",
        "triangletest1.vert.spv",
    ],
    linkopts = [
        "-lgflags",
    ],
    deps = [
        "//dvc:file",
        "//dvc:opts",
        "//dvc:terminate",
        "//spk:spock",
    ],
)

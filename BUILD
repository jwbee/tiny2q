cc_library(
    name = "recents",
    hdrs = ["recents.h"],
)

cc_library(
    name = "cache",
    hdrs = ["cache.h"],
    deps = [
        ":recents",
        "@com_google_absl//absl/container:flat_hash_map",
    ],
)

cc_test(
    name = "test",
    size = "small",
    srcs = ["test.cc"],
    deps = [
        ":cache",
        ":recents",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_binary(
    name = "benchmark",
    srcs = ["benchmark.cc"],
    malloc = "@com_google_tcmalloc//tcmalloc",
    deps = [
        ":cache",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_benchmark//:benchmark_main",
    ],
)

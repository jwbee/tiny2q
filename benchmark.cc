#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "benchmark/benchmark.h"

#include "cache.h"

using namespace std::literals;

namespace tiny2q {
// This is to prove that the idea works at all. This has to be much faster than
// main memory access or this whole scheme doesn't work. On the author's PC this
// runs in 1.8ns so that's positive.
static void BM_CachedFrequent(benchmark::State &state) {
  cache<uint32_t, bool> c(1024);
  bool v{true};
  for (int i = 0; i < 1024; i++) {
    condition cond = c.test(i, &v);
    c.observe(cond, i, v);
    cond = c.test(i, &v);
    c.observe(cond, i, v);
  }
  auto n = 0ULL;
  for (auto _ : state) {
    c.test(42, &v);
    benchmark::DoNotOptimize(n += v);
  }
}
// Register the function as a benchmark
BENCHMARK(BM_CachedFrequent);
} // namespace tiny2q

namespace {
std::default_random_engine rng{};
std::uniform_int_distribution<int> printable(32, 126);
std::uniform_int_distribution<int> lengths(1, 64);

std::string gibberish(size_t len) {
  std::string s(len, 'x');
  std::generate(s.begin(), s.end(),
                []() { return static_cast<char>(printable(rng)); });
  return s;
}

std::vector<std::string> strings(size_t count) {
  std::vector<std::string> v(count);
  std::generate(v.begin(), v.end(), []() { return gibberish(lengths(rng)); });
  return v;
}

auto the_strings = strings(32'000'000);

absl::flat_hash_map<uint32_t, std::string> map{};

bool setup_map() {
  map.reserve(the_strings.size());
  for (uint32_t i = 0; i < the_strings.size(); i++) {
    map[i] = the_strings[i];
  }
  return true;
}

bool done = setup_map();
} // namespace

namespace tiny2q {
static void BM_Uncached(benchmark::State &state) {
  // Fill access_these with N copies of the first million strings where N is
  // drawn from a power law, and at least once. This results in ~100M accesses.
  std::vector<uint32_t> access_these{};
  auto times{10000000L};
  for (uint32_t i = 0; i < 1'000'000; i++) {
    std::fill_n(std::back_inserter(access_these), times, i);
    times = std::max(1L, times / 10 * state.range(0));
  }
  // Who knows how long this is going to take.
  std::shuffle(access_these.begin(), access_these.end(), rng);

  // See how long it takes to trample all over this keyspace.
  constexpr auto needle{"prod"sv};
  for (auto _ : state) {
    auto n{0ULL};
    for (const auto &i : access_these) {
      auto s = map[i];
      benchmark::DoNotOptimize(n += (s.find(needle)));
    }
  }
}
BENCHMARK(BM_Uncached)->DenseRange(1, 9);

static void BM_Cached(benchmark::State &state) {
  // Fill access_these with N copies of the first million strings where N is
  // drawn from a power law, and at least once. This results in ~100M accesses.
  std::vector<uint32_t> access_these{};
  auto times{10000000L};
  for (uint32_t i = 0; i < 1'000'000; i++) {
    std::fill_n(std::back_inserter(access_these), times, i);
    times = std::max(1L, times / 10 * state.range(0));
  }
  // Who knows how long this is going to take.
  std::shuffle(access_these.begin(), access_these.end(), rng);

  cache<uint32_t, bool> c{state.range(1)};
  constexpr auto needle{"prod"sv};
  for (auto _ : state) {
    auto n{0ULL};
    for (const auto &i : access_these) {
      bool v;
      condition cond = c.test(i, &v);
      if (cond != condition::frequent) {
        auto s = map[i];
        v = (s.find(needle));
        c.observe(cond, i, v);
      }
      benchmark::DoNotOptimize(n += v);
    }
  }
}
BENCHMARK(BM_Cached)->ArgsProduct({benchmark::CreateDenseRange(1, 9, 1),
                                   benchmark::CreateRange(8, 131072, 4)});
} // namespace tiny2q

BENCHMARK_MAIN();
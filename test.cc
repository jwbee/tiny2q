#include "cache.h"
#include "recents.h"

#include <cstdint>

#include <gtest/gtest.h>

namespace tiny2q {
TEST(RecentsTest, Compulsory) {
  recents<uint32_t> l{32};
  EXPECT_FALSE(l.test(42));
  l.add(42);
  EXPECT_TRUE(l.test(42));
}

TEST(RecentsTest, Overflow) {
  recents<int> l{32};
  for (int i = 0; i < 32; i++)
    l.add(i);
  for (int i = 0; i < 32; i++)
    EXPECT_TRUE(l.test(i));
  l.add(42);
  EXPECT_FALSE(l.test(0));
  EXPECT_TRUE(l.test(42));
  for (int i = 1; i < 32; i++)
    EXPECT_TRUE(l.test(i));
}

TEST(CacheTest, Compulsory) {
  constexpr size_t count = 16;
  cache<int, bool> c{count};
  bool v{false};

  condition cond = c.test(42, &v);
  EXPECT_EQ(condition::absent, cond);
  EXPECT_FALSE(v);

  c.observe(cond, 42, true);
  cond = c.test(42, &v);
  EXPECT_EQ(cond, condition::recent);
  EXPECT_FALSE(v);

  c.observe(cond, 42, true);
  cond = c.test(42, &v);
  EXPECT_EQ(cond, condition::frequent);
  EXPECT_TRUE(v);

  // This should not evict our frequent key.
  for (int i = 0; i < count; i++) {
    cond = c.test(i, &v);
    EXPECT_EQ(cond, condition::absent);
    c.observe(cond, i, v);
  }

  cond = c.test(42, &v);
  EXPECT_EQ(cond, condition::frequent);
  EXPECT_TRUE(v);

  // This should evict the former frequent key.
  for (int i = count; i < count + count; i++) {
    cond = c.test(i, &v);
    c.observe(cond, i, v);
    cond = c.test(i, &v);
    c.observe(cond, i, v);
    cond = c.test(i, &v);
    EXPECT_EQ(cond, condition::frequent);
  }

  // The live frequent keys should now be [16, 32).
  cond = c.test(42, &v);
  EXPECT_EQ(cond, condition::absent);

  // Repeatedly accessing 16 and 31 should not evict further.
  for (int i = 0; i < 1024; i++) {
    cond = c.test(count + 15, &v);
    EXPECT_EQ(cond, condition::frequent);
    cond = c.test(count, &v);
    EXPECT_EQ(cond, condition::frequent);
  }
  for (int i = count; i < count + count; i++) {
    cond = c.test(i, &v);
    EXPECT_EQ(cond, condition::frequent);
  }
}

} // namespace tiny2q
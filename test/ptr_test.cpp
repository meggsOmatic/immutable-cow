#include "cow/ptr.h"
#include <gtest/gtest.h>

namespace {
struct point2i {
  int x, y;
};
}

TEST(CowPtr, Int) {
  cow::ptr<int> i = cow::make<int>(1);
  EXPECT_TRUE(i);
  EXPECT_NE(i.get(), nullptr);
  EXPECT_EQ(*i, 1);
  EXPECT_EQ(i.use_count(), 1);

  cow::ptr<int> j{ i };
  EXPECT_TRUE(j);
  EXPECT_NE(j.get(), nullptr);
  EXPECT_EQ(*j, 1);
  EXPECT_EQ(i.use_count(), 2);
  EXPECT_EQ(j.use_count(), 2);
  EXPECT_EQ(i.get(), j.get());
  EXPECT_EQ(i, j);
  
  {
    cow::ptr<int> k;
    EXPECT_FALSE(k);
    EXPECT_EQ(k, nullptr);
    k = i;
    EXPECT_TRUE(k);
    EXPECT_NE(k.get(), nullptr);
    EXPECT_EQ(*k, 1);
    EXPECT_EQ(i.use_count(), 3);
    EXPECT_EQ(j.use_count(), 3);
    EXPECT_EQ(k.use_count(), 3);
    EXPECT_EQ(i.get(), k.get());
    EXPECT_EQ(i, k);
  }
  
  EXPECT_EQ(i.use_count(), 2);
  EXPECT_EQ(j.use_count(), 2);
  EXPECT_EQ(i.get(), j.get());
  EXPECT_EQ(i, j);
  
  *--j = 2;
  EXPECT_EQ(i.use_count(), 1);
  EXPECT_EQ(j.use_count(), 1);
  EXPECT_NE(i.get(), j.get());
  EXPECT_NE(i, j);
  EXPECT_EQ(*i, 1);
  EXPECT_EQ(*j, 2);

  cow::ptr<int> m{ j };
  EXPECT_TRUE(j);
  EXPECT_TRUE(m);
  EXPECT_EQ(m, j);
  cow::ptr<int> n{ std::move(j) };
  EXPECT_TRUE(n);
  EXPECT_FALSE(j);
  EXPECT_EQ(m, n);
  EXPECT_NE(m, j);
  EXPECT_NE(n, j);
  EXPECT_EQ(j.use_count(), 0);
  EXPECT_EQ(m.use_count(), 2);
  EXPECT_EQ(n.use_count(), 2);
  n = nullptr;
  EXPECT_EQ(m.use_count(), 1);
  EXPECT_EQ(n.use_count(), 0);
}

TEST(CowPtr, Struct) {
  auto i = cow::make<point2i>(1, 2);
  auto j = i;
  EXPECT_EQ(i, j);
  EXPECT_EQ(i->x, 1);
  EXPECT_EQ(i->y, 2);
  i--->x += 10;
  EXPECT_NE(i, j);
  EXPECT_EQ(i->x, 11);
  EXPECT_EQ(i->y, 2);
  EXPECT_EQ(j->x, 1);
  EXPECT_EQ(j->y, 2);
}


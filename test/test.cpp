#include "cow/ptr.h"
#include <gtest/gtest.h>

TEST(Cow, Hi) {
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
}


#include "cow/ptr.h"
#include <gtest/gtest.h>

namespace {
struct point2i {
  int x, y;
};

struct Base {
  virtual ~Base() = default;
  int b{1};
};

struct Derived : public Base {
  int d{2};
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
  EXPECT_EQ(n, nullptr);
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

TEST(CowPtr, ImplicitCasts) {
  auto b = cow::make<Base>();
  EXPECT_EQ(b.type_info(), typeid(Base));

  auto d = cow::make<Derived>();
  EXPECT_EQ(d.type_info(), typeid(Derived));

  cow::ptr<Base> b2{d};
  EXPECT_EQ(d, b2);
  EXPECT_EQ(b2.type_info(), typeid(Derived));

  cow::ptr<Base> b3;
  EXPECT_EQ(b3.type_info(), typeid(nullptr));
  b3 = d;
  EXPECT_NE(d, nullptr);
  EXPECT_EQ(d, b3);
  EXPECT_EQ(b3.type_info(), typeid(Derived));

  cow::ptr<Derived> d2 = d;
  EXPECT_EQ(d, d2);

  cow::ptr<Base> b4{std::move(d2)};
  EXPECT_EQ(d2, nullptr);
  EXPECT_NE(b2, nullptr);
  EXPECT_EQ(b2, d);

  d2 = d;
  EXPECT_NE(d, nullptr);
  EXPECT_NE(d2, nullptr);
  b3 = std::move(d2);
  EXPECT_EQ(d2, nullptr);
  EXPECT_NE(b3, nullptr);
  EXPECT_EQ(d, b3);

  // None of these should compile.
  // d = b;
  // d = std::move(b);
  // cow::ptr<Derived> d4(b);
  // cow::ptr<Derived> d5(std::move(b));
}

TEST(CowPtr, ExplicitCasts) {
  cow::ptr<Base> b = cow::make<Derived>();
  EXPECT_EQ(b.type_info(), typeid(Derived));

  auto d1 = b.cast<Derived>();
  EXPECT_EQ(d1, b);
  EXPECT_EQ(b.use_count(), 2);
  auto d2 = b.move_cast<Derived>();
  EXPECT_EQ(d1, d2);
  EXPECT_EQ(b, nullptr);
  EXPECT_EQ(b.use_count(), 0);
  EXPECT_EQ(d1.use_count(), 2);

  b = d2.cast<Base>();
  EXPECT_EQ(b, d1);
  EXPECT_EQ(b.use_count(), 3);
  b = d2.cast<Derived>();
  EXPECT_EQ(b, d1);
  EXPECT_EQ(b.use_count(), 3);
  b = d2.move_cast<Base>();
  EXPECT_EQ(b, d1);
  EXPECT_EQ(b.use_count(), 2);

  d1 = b.dynamic<Derived>();
  EXPECT_EQ(b, d1);
  EXPECT_EQ(d1.use_count(), 2);
  d2 = b.move_dynamic<Derived>();
  EXPECT_EQ(d1, d2);
  EXPECT_EQ(b, nullptr);
  EXPECT_NE(d2, nullptr);
  EXPECT_EQ(d1.use_count(), 2);
  b = d2;
  EXPECT_EQ(d1.use_count(), 3);
  d2 = b.move_dynamic<Derived>();
  EXPECT_EQ(d1.use_count(), 2);

  b = cow::make<Base>();
  d1 = b.dynamic<Derived>();
  EXPECT_NE(b, nullptr);
  EXPECT_EQ(d1, nullptr);
  EXPECT_EQ(b.use_count(), 1);
  EXPECT_EQ(d1.use_count(), 0);
  EXPECT_EQ(d2.use_count(), 1);

  d1 = b.move_dynamic<Derived>();
  EXPECT_EQ(b, nullptr);
  EXPECT_EQ(d1, nullptr);
}

#include "cow/path.h"
#include <gtest/gtest.h>

namespace {
struct tree {
  int value;
  cow::ptr<tree> left, right;
};

void replaceInPlace(cow::spot<tree>&& node, int oldVal, int newVal) {
  if (!node) {
    return;
  }

  if (node->value == oldVal) {
    node--->value = newVal;
  }

  replaceInPlace(node.step([](const tree& from) { return &from.left; }), oldVal,
          newVal);
  replaceInPlace(node.step(&node->right),
                 oldVal,
          newVal);
}
}  // namespace

TEST(CowPath, TreeWithSpots) {
  cow::ptr<tree> a = cow::make<tree>(
      1, cow::make<tree>(2, cow::make<tree>(3), cow::make<tree>(4)),
      cow::make<tree>(5, cow::make<tree>(6), cow::make<tree>(7)));
  cow::ptr<tree> b = a;
  EXPECT_EQ(a, b);
  EXPECT_EQ(a.use_count(), 2);
  EXPECT_EQ(a->value, 1);
  EXPECT_EQ(a->left, b->left);
  EXPECT_EQ(a->left->value, 2);
  EXPECT_EQ(a->left.use_count(), 1);
  EXPECT_EQ(a->right, b->right);
  EXPECT_EQ(a->right->value, 5);
  EXPECT_EQ(a->right.use_count(), 1);
  EXPECT_EQ(a->right->left, b->right->left);
  EXPECT_EQ(a->right->left->value, 6);

  replaceInPlace(cow::root_spot(&a), 6, 16);

  EXPECT_NE(a, b);
  EXPECT_EQ(a.use_count(), 1);
  EXPECT_EQ(b.use_count(), 1);
  EXPECT_EQ(a->value, 1);
  EXPECT_EQ(b->value, 1);
  EXPECT_EQ(a->left, b->left);
  EXPECT_EQ(a->left->value, 2);
  EXPECT_EQ(a->left.use_count(), 2);
  EXPECT_NE(a->right, b->right);
  EXPECT_EQ(a->right.use_count(), 1);
  EXPECT_EQ(b->right.use_count(), 1);
  EXPECT_EQ(a->right->value, 5);
  EXPECT_EQ(b->right->value, 5);
  EXPECT_NE(a->right->left, b->right->left);
  EXPECT_EQ(a->right->left.use_count(), 1);
  EXPECT_EQ(b->right->left.use_count(), 1);
  EXPECT_EQ(a->right->left->value, 16);
  EXPECT_EQ(b->right->left->value, 6);
  EXPECT_EQ(a->right->right, b->right->right);
  EXPECT_EQ(a->right->right.use_count(), 2);
  EXPECT_EQ(a->right->right->value, 7);
}

TEST(CowPath, TreeWithPath) {
  cow::ptr<tree> a = cow::make<tree>(
      1, cow::make<tree>(2, cow::make<tree>(3), cow::make<tree>(4)),
      cow::make<tree>(5, cow::make<tree>(6), cow::make<tree>(7)));
  cow::ptr<tree> b = a;

  auto left = [](const tree& from) { return &from.left; };
  auto right = [](const tree& from) { return &from.right; };
  cow::path<tree> pathToSix = cow::make_path(&a, right, left);

  EXPECT_EQ(a, b);
  EXPECT_EQ(a.use_count(), 2);
  EXPECT_EQ(a->value, 1);
  EXPECT_EQ(a->left, b->left);
  EXPECT_EQ(a->left->value, 2);
  EXPECT_EQ(a->left.use_count(), 1);
  EXPECT_EQ(a->right, b->right);
  EXPECT_EQ(a->right->value, 5);
  EXPECT_EQ(a->right.use_count(), 1);
  EXPECT_EQ(a->right->left, b->right->left);
  EXPECT_EQ(a->right->left->value, 6);

  EXPECT_EQ(pathToSix->value, 6);
  pathToSix--->value += 10;

  EXPECT_NE(a, b);
  EXPECT_EQ(a.use_count(), 1);
  EXPECT_EQ(b.use_count(), 1);
  EXPECT_EQ(a->value, 1);
  EXPECT_EQ(b->value, 1);
  EXPECT_EQ(a->left, b->left);
  EXPECT_EQ(a->left->value, 2);
  EXPECT_EQ(a->left.use_count(), 2);
  EXPECT_NE(a->right, b->right);
  EXPECT_EQ(a->right.use_count(), 1);
  EXPECT_EQ(b->right.use_count(), 1);
  EXPECT_EQ(a->right->value, 5);
  EXPECT_EQ(b->right->value, 5);
  EXPECT_NE(a->right->left, b->right->left);
  EXPECT_EQ(a->right->left.use_count(), 1);
  EXPECT_EQ(b->right->left.use_count(), 1);
  EXPECT_EQ(a->right->left->value, 16);
  EXPECT_EQ(b->right->left->value, 6);
  EXPECT_EQ(a->right->right, b->right->right);
  EXPECT_EQ(a->right->right.use_count(), 2);
  EXPECT_EQ(a->right->right->value, 7);
}

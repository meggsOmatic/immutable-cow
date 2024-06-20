# Immutable CoW

This is an incomplete in-progress personal project to make some immutable copy-on-write data structures in C++. These are also known as pure-functional or [persistent data structures](https://en.wikipedia.org/wiki/Persistent_data_structure). *(Note that the "persistence" in this case has nothing to do with serializing data to files. It refers to the abilty to have old versions of data persist after new versions are created and modified.)*

> [!CAUTION]
> **This not quite ready for general use, but I'm actively working on it.**

<img src="https://github.com/meggsOmatic/immutable-cow/assets/5649419/38b58176-cdbe-4f17-8817-bbaedb70238b" height=50% width=50% >

The core of the design starts with `cow::ptr<T>`, which reference-counts any object it points to. (A control block is allocated behind-the-scenes for this, just as with `std::make_shared`.)
```cpp
struct point2i { int x, y; };
cow::ptr<int> a = cow::make<point2i>(3, 4);
cow::ptr<int> b = a;
assert(a.use_count() == 2);
assert(b->y == 4);
```

Modification of the pointed-to object is only allowed if the reference count is ***exactly one***. The `operator *` and `operator ->` methods and `get()` methods are `const`, and return a `const` reference or pointer to the pointed-to object. To modify the object, use `cow::ptr<T>::write()`, which returns a non-const pointer. Similar to (Arc::make_mut)[https://doc.rust-lang.org/std/sync/struct.Arc.html#method.make_mut] in Rust, `write()` will return a pointer to the existing object if the refcount is 1. If the refcount is greated than 1, `write()` will clone the existing object and replace its internal reference with the clone, which will be modifiable because its refcount is 1.

```cpp
cow::ptr<int> a = cow::make<point2i>(3, 4);
assert(a.use_count() == 1);

cow::ptr<int> b = a; // additional reference to the same object
assert(a == b && a.use_count() == 2 && b.use_count() == 2);

cout << b->x; // read-only access to const object
// b->x += 10; // COMPILE ERROR, const point2i
b.write()->x += 10; // forces a clone of the point2i
assert(a != b && a.use_count() == 1 && b.use_count() == 1);

b.write()->y += 10; // no clone this time
```

As a stylistic conceit, `cow::ptr<T>::operator--` is a synonym for `write()`. Think of it as "lower the refcount down to 1" if you like. It enables these idiomatic patterns:
```cpp
b--->x = 5;
point2i* p = --b;
```

Because `write()` must be able to modify its own internal pointer, it must itself be a non-const method. Thus you can only objects at the end of a chain of modifiable objects.
```cpp
struct tree { int value; cow::ptr<tree> left, right; };
cow::ptr<tree> root = cow::make<tree>(1, cow::make<tree>(2), cow::make<tree>(3));
// root->left--->value += 10; // COMPILE ERROR, root->left is accessed via a const tree*.
root--->left--->value += 10; // All good.
```

This lends itself naturally to combining the thread-safety of immutable pure-functional data structures, but still allows efficient minimal overhead changes when a single thread makes multiple modifications to the same object.
```cpp
cow::ptr<tree> root = cow::make<tree>(1, cow::make<tree>(2), cow::make<tree>(3));
cow::ptr<tree> treeCopy = root; // Shares the same root node
assert(root.use_count() == 2);
// The root node has the only references to left and right
assert(root->left.use_count() == 1 && root->right.use_count() == 1);

root--->value += 1; // New root, both roots shares left and right
assert(root != treeCopy && root.use_count() == 1 && treeCopy.use_count() == 1);
assert(root->left == treeCopy->left  && root->left.use_count() == 2);
assert(root->right == treeCopy->right && root->right.use_count() == 2);

root--->left--->value += 1; // root is unique so unchanged, clones left, still shares right
assert(root->left != treeCopy->left && && root->left.use_count() == 1);
assert(root->right == treeCopy->right && root->right.use_count() == 2);
```

In the above example, note that: ***At no point was anything directly or indirectly pointed-to by treeCopy modified in any way.*** You could capture a copy of `root` by value in a lmabda, or hand it off by value to a thousand threads which would perform read-only computations based on an immutable snapsot of the entire tree. The only cost of that would be cost of incrementing a refcount. Meanwhile everything pointed to by the original `root` can be updated incrementally and asynchronously. Those modifications wouldn't affecting anyone else's view of that snapshot, and the cost would be on-demand clones of the *only* the parts which were modified.

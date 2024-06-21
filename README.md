# Immutable CoW 🐄
<img src="https://github.com/meggsOmatic/immutable-cow/assets/5649419/38b58176-cdbe-4f17-8817-bbaedb70238b" height=25% width=25% align="right"/>

This is a (very very) incomplete in-progress personal project to make some **Immutable Copy-on-Write** data structures in C++. These are also known as pure-functional or [persistent data structures](https://en.wikipedia.org/wiki/Persistent_data_structure). *(Note that the "persistence" in this case has nothing to do with serializing data to files. It refers to the abilty to have old versions of data persist after new versions are created and modified.)*

> [!CAUTION]
> This is not production-grade yet. Many containers are still to be implemented, and APIs may change without warning.

## 🐮Refcounted pointers 
The core of the design starts with `cow::ptr<T>`, which reference-counts any object it points to. (A control block is allocated behind-the-scenes for this, just as with `std::make_shared`.)
```cpp
struct point2i { int x, y; };
cow::ptr<int> a = cow::make<point2i>(3, 4);
cow::ptr<int> b = a;
assert(a.use_count() == 2);
assert(b->y == 4);
```

### 🐮Copy-on-Write
Modification of the pointed-to object is only allowed if the reference count is ***exactly one***. The `operator *` and `operator ->` methods and `get()` methods are `const`, and return a `const` reference or pointer to the pointed-to object. To modify the object, use `cow::ptr<T>::write()`, which returns a non-const pointer. Similar to (Arc::make_mut)[https://doc.rust-lang.org/std/sync/struct.Arc.html#method.make_mut] in Rust, `write()` will return a pointer to the existing object if the refcount is 1. If the refcount is greated than 1, `write()` will clone the existing object and replace its internal reference with the clone, which will be modifiable because its refcount is 1.

```cpp
cow::ptr<int> a = cow::make<point2i>(3, 4);
assert(a.use_count() == 1);

// additional reference to the same object
cow::ptr<int> b = a;
assert(a == b && a.use_count() == 2 && b.use_count() == 2);

// read-only access to const object
cout << b->x;

// COMPILE ERROR, const point2i*
// b->x += 10;

// force a clone of the point2i
b.write()->x += 10;

// a and b no longer point to the same object
assert(a != b && a.use_count() == 1 && b.use_count() == 1);
assert(a->x == 3 && b->x == 13);

// Because b has the only reference, this will not make a second clone.
b.write()->y += 10;
```

### 🐮Operators with style
As a stylistic conceit, `cow::ptr<T>::operator--` is a synonym for `write()`. Think of it as "lower the refcount down to 1" if you like. It enables some "cute" idiomatic patterns, especially when back-to-back with `->`.
```cpp
b--->x = 5;
point2i* p = --b;
```
I find it convenient; some may find it horrifying. Users of the library are free to choose a local coding standard of never using it.

### 🐮Writes may modify the pointer itself
Because `write()` must be able to modify its own internal pointer to point to a new cloned object, it must itself be a non-const method. Thus you can only modify objects at the end of a chain of modifiable objects.
```cpp
struct tree { int value; cow::ptr<tree> left, right; };
cow::ptr<tree> root = cow::make<tree>(1, cow::make<tree>(2), cow::make<tree>(3));

// COMPILE ERROR, root->left is a a `const cow::ptr<tree>&` and so we can't use write().
// root->left--->value += 10;

// Get a non-const reference to left, so we can get a non-const reference to left->value.
root--->left--->value += 10;
```

### 🐮Trees of immutable objects
This lends itself naturally to combining the thread-safety of immutable pure-functional data structures, but still allows efficient minimal overhead changes when a single thread makes multiple modifications to the same object.
```cpp
cow::ptr<tree> root = cow::make<tree>(1, cow::make<tree>(2), cow::make<tree>(3));

// root and treeCopy share the same root node...
cow::ptr<tree> treeCopy = root;
assert(root.use_count() == 2);

// ...and the root node has the only references to left and right
assert(root->left.use_count() == 1 && root->right.use_count() == 1);

 // New root, both trees shares left and right nodes
root--->value += 10;
assert(root != treeCopy && root.use_count() == 1 && treeCopy.use_count() == 1);
assert(root->left == treeCopy->left  && root->left.use_count() == 2);
assert(root->right == treeCopy->right && root->right.use_count() == 2);

// root is unique so unchanged, clones left, still shares right
root--->left--->value += 10;
assert(root->left != treeCopy->left && && root->left.use_count() == 1);
assert(root->right == treeCopy->right && root->right.use_count() == 2);
```
![image](https://github.com/meggsOmatic/immutable-cow/assets/5649419/492b97f0-b8d7-4308-94b8-55768aae3b20)

> [!NOTE]
> In the above example, note that ***nothing directly or indirectly pointed-to by treeCopy was ever modified in any way.*** This is the central value proposition of this entire approach. You could capture a copy of `root` by value in a lambda, or hand it off by value to a thousand threads to perform read-only computations on an immutable snapsot of a large structure. The only cost of that would be cost of incrementing a refcount to the root node. Meanwhile everything pointed to by the original `root` can be updated incrementally and asynchronously. Those modifications wouldn't affecting anyone else's view of that snapshot, and the cost would be on-demand clones of the *only* the parts which were modified.

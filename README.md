# Immutable CoW
<img src="https://github.com/meggsOmatic/immutable-cow/assets/5649419/38b58176-cdbe-4f17-8817-bbaedb70238b" height=25% width=25% align="right"/>

This is a (very very) incomplete in-progress personal project to make some **Immutable Copy-on-Write** data structures in C++. These are also known as pure-functional or [persistent data structures](https://en.wikipedia.org/wiki/Persistent_data_structure). *(Note that the "persistence" in this case has nothing to do with serializing data to files. It refers to the abilty to have old versions of data persist after new versions are created and modified.)*

> [!CAUTION]
> This is not production-grade yet. Many containers are still to be implemented, and APIs may change without warning.

## 🐮 Refcounted pointers 🐮
The core of the design starts with `cow::ptr<T>`, which reference-counts any object it points to. (A control block is allocated behind-the-scenes for this, just as with `std::make_shared`.)
```cpp
struct point2i { int x, y; };
cow::ptr<int> a = cow::make<point2i>(3, 4);
cow::ptr<int> b = a;
assert(a.use_count() == 2);
assert(b->y == 4);
```

### 🐄 Copy-on-Write
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

### 🐄 Operators with style
As a stylistic conceit, `cow::ptr<T>::operator--` is a synonym for `write()`. Think of it as "lower the refcount down to 1" if you like. It enables some "cute" idiomatic patterns, especially when back-to-back with `->`.
```cpp
b--->x = 5;
point2i* p = --b;
```
I find it convenient; some may find it horrifying. Users of the library are free to choose a local coding standard of never using it.

### 🐄 Writes may modify the pointer itself
Because `write()` must be able to modify its own internal pointer to point to a new cloned object, it must itself be a non-const method. Thus you can only modify objects at the end of a chain of modifiable objects.
```cpp
struct tree { int value; cow::ptr<tree> left, right; };
cow::ptr<tree> root = cow::make<tree>(1, cow::make<tree>(2), cow::make<tree>(3));

// COMPILE ERROR, root->left is a a `const cow::ptr<tree>&` and so we can't use write().
// root->left--->value += 10;

// Get a non-const reference to left, so we can get a non-const reference to left->value.
root--->left--->value += 10;
```

### 🐄 Trees of immutable objects
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

> [!IMPORTANT]
> In the above example, note that ***nothing reachable through `treeCopy` was ever modified in any way.*** First copies were made, and then the copies were modified.
> 
> This is the central value proposition of this entire approach. You could capture a copy of `root` by value in a lambda, or hand it off by value to a thousand threads to perform read-only computations on an immutable snapsot of a large structure. The only cost of that would be cost of incrementing a refcount to the root node.
>
> Meanwhile everything pointed to by the original `root` can be updated incrementally and asynchronously. Those modifications wouldn't affecting anyone else's view of that snapshot, and the cost would be on-demand clones of the *only* the parts which were modified.

## 🐮 Knowing your place 🐮

This library takes the [path copying](https://en.wikipedia.org/wiki/Persistent_data_structure#Path_copying) approach to modifying structures. In practice, there's a very common pattern where you'll search for the place where something should be changed, and then you'll (possibly) make a change at that location. In order to do that, you'll need a writeable copy of the object containing that data. But in order to swap in a writeable copy of that object, you'll need a writeable copy of the parent object that points to it, and so on up the chain to the root.

We could make modifiable copies as we initially traverse to the point of modification, and `root--->parent--->child--->value` does that easily when we *know* we need to make a change. But when we're scanning a data structure, and we not decide if we want to update something until we've found it, that's wasteful. We need a way to retrace our steps. Non-persistent data structures will frequently maintaining a parent pointer in their nodes to facilitate this. But that doesn't work for us, because a node may be shared by multiple copies of its parents.

Instead, we need to keep track of how we got to our point of modification, so that we can retrace and make modifiable copies as needed. There are two classes to help with this: `cow::spot<T>` and `cow::path<T>`.

### 🐄 Spotted CoWs

The base mechanism for this is `cow::spot<T>`. Think of it as a wrapper that points to a `cow::ptr<T>` and re-exposes its API:
```cpp
void move_x_value(cow::spot<point2i>&& spot) {
  if (spot) {
    std::cout << spot->x << " " << spot->y << std::endl;
    spot--->x += 10;
  }
}
```

#### root_spot
There are several types of `cow::spot`s available. The most basic is a `cow::root_spot<T>`, which is simply initialized as a pointer to a *non-const* `cow::ptr<T>`. You'll generally start with one of these pointing at the root of your data structure:
```cpp
auto p = cow::make<point2i>(5, 6);
move_x_value(cow::root_spot(&p));
```

#### next_spot (and subclasses)
The second type of spot you'll frequently use is some variant of `cow::next_spot<FromObject, ToObject>`. This is a spot that knows internally how to look at a parent object of type `FromObject` and return pointer to a `cow::ptr<ToObject>` within that object. It has to be chained off of a `cow::spot<FromObject>`, and when you ask for writeable access it will ask its parent for writeable access if necessary. A hand-wavey details-omitted example:
```cpp
struct rect2i {
  cow::ptr<point2i> topLeft;
  cow::ptr<point2i> bottomRight;
};

auto r = cow::make<rect2i>(
  cow::make<point2i>(1, 2),
  cow::make<point2i>(5, 20));

root_spot<rect2i> rectSpot(&r);

// This next line is hand-wavey and won't compile (see below).
next_spot<rect2i, point2i> pointSpot(rs, (something...) &topLeft);

// This is equivalent to r--->topLeft--->x += 1;
pointSpot--->x += 1;
```

When you ask a `next_spot` to give you a writeable `ToObject*`, it will in turn ask its parent `spot<FromObject>` to give it a writeable `FromObject*`, so it can get non-const access to a `cow::ptr<ToObject>` within that object, which it needs in order to call `cow::ptr<ToObject>::write()`. In turn, if the parent is another `next_spot` it will ask *its* parent for a writeable object so it can get non-const access to a `cow::ptr<FromObject>` within *that* object, and so on until it gets to a `root_spot`.

Caching is used within the chains of spots, so that multiple write accesses won't re-traverse the entire chain.

#### lambda_spot

`cow::next_spot` is actually an abstract class. The details of how to get from the parent object to the child pointer are in subclasses that a implement a virtual `const cow::ptr<ToObject>& step(const FromObject&)` function. The most straightforward just calls a user-supplied lamda:

```cpp
cow::lambda_spot<recti, point2i, (lambda-type)> pointSpot(
  rs, [](const rect2i& from) { return &from.topLeft; });
```

With that declared, `cout << pointSpot->x` will internally do something a bit like: `lambda(*rectSpot)->get()->x`, or more eplicitly:
```cpp
const rect2i& fromObjectRef = *rectSpot;
const cow::ptr<point2i>* pointerToPointerInFromObject = lambda(fromObject);
const point2i* toObjectPtr = pointerToPointerInFromObject->get();
cout << toObjectPtr->x;
```

Write access via `pointSpot--->x += 1` is a little more complicated, but means something like:
```cpp
// This may return a DIFFERENT rect2i than we were using.
rect2i* writeableFromObject = rectSpot.write();

// So we re-invoke the lambda to get a pointer within that new object.
// The lambda returns a const pointer so it works in the read-only case...
const cow::ptr<point2i>* constPointerInFromObject =
  lambda(*writeableFromObjectPtr);

// ...but this cast is safe, because it came from writeableFromObject
// which we know was non-const.
cow::ptr<point2i>* writeablePointerInFromObject =
  const_cast<cow::ptr<point2i>*>(constPointerToPointerInFromObject);

// This may (potentially) turn modify *writeablePointerInFromObject to
// point to a new clone of the point2i.
point2i* writeableObject = writeablePointerInFromObject->write();

// And now we can write to the value.
writeableObject->x += 1;
```

(Null checks and caching elided for brevity.) The important thing to keep in mind is that getting write access within a chain of spots may change a parent object, which in turn means re-finding the `cow::ptr` to the child object.

#### offset_spot

An easier-to-use (but much less explicit) form is `offset_spot`. Rather than a general-purpose lambda, it just refers to a `cow::ptr` in the parent object via a fixed offset to a fixed field. You can initialize it by just passing in a pointer to within the parent object:
```cpp
cow::offset_spot<rect2i, point2i> topLeftSpot(rectSpot, &rectSpot->topLeft);
cow::offset_spot<rect2i, point2i> bottomRightSpot(rectSpot, &rectSpot->bottomRight);
```

Similar to the lambda version, this doesn't store the actual pointer. It remembers at the offset of the pointer within the "from" object, and will re-apply that offset when the parent spot might change to a new object.

#### spot::step()

That's all a lot of declaration, and you still need the correct lambda type. Most of the time, given a parent spot you can call its `step()` method to return a useful child spot:
```cpp
// Returns a cow::lambda_spot<rect2i, point2i, (lambda-type)>
auto topLeftSpot = rectSpot.step([](const recti& from) { return &from.topLeft; });

// Returns a cow::offset_spot<rect2i, point2i>
auto bottomRightSpot = rectSpot.step(&rectSpot->bottomRight);
```

Putting it all together, it's easy to do something like:
```cpp
struct binary_tree {
  int key;
  std::string value;
  cow::ptr<tree> left;
  cow::ptr<tree> right;
};

void search_key_and_replace_value(cow::spot<binary_tree>&& spot,
                                  int key,
                                  const std::string& newValue) {
  if (!spot) { return; }

  if (key == spot->key) {
    spot--->value = newValue;
  } else {
    search_key_and_replace_value(
      spot.step(key < spot->key ? &spot->left
                                : &spot->right),
      key,
      newValue);
  }
}
```

### 🐄 Down the CoW path

> [!CAUTION] I expect the `path` API will change based on experience using it.

The various `cow::spot`s are great on a recursive stack, but sometimes you need to encapsulate an arbitraray-length chain of spots as a single object. A typical example would be an iterator into a tree structure. This is done with a `cow::path<T>`, which assumes a linear chain of same-typed objects. More to come...

#pragma once

#include "cow/ptr.h"

#include <memory>
#include <vector>

namespace cow {

template <typename ObjectType>
class spot {
 public:
  using object_type = ObjectType;

  explicit operator bool() const noexcept { return here && *here; }

  const ObjectType& operator*() const noexcept {
    assert(here);
    return **here;
  }

  const ObjectType* get() const noexcept {
    return here ? here->get() : nullptr;
  }

  const ObjectType* operator->() const noexcept {
    assert(here && *here);
    return here->get();
  }

  virtual ObjectType* write() noexcept = 0;

  ObjectType* operator--(int) { return write(); }
  ObjectType* operator--() { return write(); }

  virtual const std::type_info& type_info() const noexcept {
    return typeid(object_type);
  }

  size_t use_count() const noexcept { return here ? here->use_count() : 0; }

  template <typename StepFunc>
  auto step_from(StepFunc&& func) noexcept;

 protected:
  explicit spot(const ptr<ObjectType>* where) : here(where) {}
  spot(const spot&) = delete;
  spot& operator=(const spot&) = delete;
  spot(spot&& other) noexcept : here(other) { other.here = nullptr; }
  spot& operator=(spot&& other) noexcept {
    assert(this != &other);  // virtual class; derived must check this
    here = other.here;
    other.here = nullptr;
    return *this;
  }

  const ptr<ObjectType>* here{nullptr};
};

template <typename ObjectType>
class root_spot final : public spot<ObjectType> {
 public:
  root_spot() noexcept : spot(nullptr) {}
  explicit root_spot(ptr<ObjectType>* where) noexcept
      : spot<ObjectType>(where) {}

  root_spot(root_spot&& other) noexcept = default;
  root_spot& operator=(root_spot&& other) noexcept {
    if (this != other) {
      spot<ObjectType>::operator=(std::move(other));
    }
  }

  virtual ObjectType* write() noexcept override {
    return this->here ? --*const_cast<ptr<ObjectType>*>(this->here) : nullptr;
  }
};

template <typename FromObjectType, typename ToObjectType>
class next_spot : public spot<ToObjectType> {
 public:
  ToObjectType* write() noexcept final override {
    if (!hasWritten) {
      hasWritten = true;

      if (fromSpot == nullptr) {
        return nullptr;
      }

      const FromObjectType* oldObject = fromSpot->get();
      FromObjectType* newObject = fromSpot->write();

      if (newObject == nullptr) {
        this->here = nullptr;
      } else if (newObject != oldObject) {
        this->here = step(*newObject);
      } else {
        assert(this->here == step(*newObject));
      }
    }

    return this->here ? --*const_cast<ptr<ToObjectType>*>(this->here) : nullptr;
  }

 protected:
  next_spot(spot<FromObjectType>& from, const ptr<ToObjectType>* where) noexcept
      : spot<ToObjectType>(where), fromSpot(&from), hasWritten(false) {}

  next_spot(next_spot&& other) noexcept
      : spot<ToObjectType>(std::move(other)),
        fromSpot(other.fromSpot),
        hasWritten(other.hasWritten) {
    other.fromSpot = nullptr;
    other.hasWritten = false;
  }

  next_spot& operator=(next_spot&& other) noexcept {
    spot<ToObjectType>::operator=(std::move(other));

    fromSpot = other.fromSpot;
    other.fromSpot = nullptr;

    hasWritten = other.hasWritten;
    other.hasWritten = false;

    return *this;
  }

  virtual const ptr<ToObjectType>* step(
      const FromObjectType& from) const noexcept = 0;

 private:
  spot<FromObjectType>* fromSpot;
  bool hasWritten;
};

template <typename FromObjectType, typename StepFunc, typename ToObjectType>
class lambda_spot final : public next_spot<FromObjectType, ToObjectType> {
 public:
  lambda_spot(spot<FromObjectType>& from, StepFunc&& stepFunc) noexcept
      : next_spot<FromObjectType, ToObjectType>(
            from,
            from ? stepFunc(*from) : nullptr),
        stepFunc(std::forward<StepFunc>(stepFunc)) {}

  lambda_spot(lambda_spot&& other) noexcept = default;

  lambda_spot& operator=(lambda_spot&& other) noexcept {
    if (this != &other) {
      next_spot::operator=(std::move(other));
      stepFunc = std::move(other.stepFunc);
    }
    return *this;
  }

 protected:
  const ptr<ToObjectType>* step(
      const FromObjectType& from) const noexcept override {
    return stepFunc(from);
  }

 private:
  StepFunc stepFunc;
};

template <typename FromObjectType>
template <typename StepFunc>
auto spot<FromObjectType>::step_from(StepFunc&& func) noexcept {
  using ToCowPtrType = std::remove_const_t<
      std::remove_pointer_t<decltype(func(*(const FromObjectType*)nullptr))>>;
  using ToObjectType = typename ToCowPtrType::object_type;
  return lambda_spot<FromObjectType, StepFunc, ToObjectType>(
      *this, std::forward<StepFunc>(func));
}

// TODO: Write a good scratch allocator and use it here :-)
template <typename ObjectType>
class path : public spot<ObjectType> {
 public:
  path() {}

  template <typename RootSpotType = root_spot<ObjectType>, typename... ArgTypes>
  explicit path(ArgTypes&&... args) : spot(nullptr) {
    spots.emplace_back(new RootSpotType(std::forward<ArgTypes>(args)...));
    this->here = spots.back()->here;
  }

  size_t size() const noexcept { return spots.size(); }

  void pop_back(size_t count = 1) noexcept {
    assert(count >= spots.size());
    spots.resize(spots.size() - count);
    this->here = spots.empty() ? nullptr : spots.back()->here;
  }

  void resize(size_t newSize) noexcept {
    assert(newSize <= spots.size());
    spots.resize(newSize);
    this->here = newSize > 0 ? spots.back()->here : nullptr;
  }

  template <typename StepFunc>
  void add_lambda(StepFunc&& stepFunc) noexcept {
    assert(!spots.empty());
    spots.emplace_back(new lambda_spot<ObjectType, StepFunc, ObjectType>(
        *spots.back(), std::forward<StepFunc>(stepFunc)));
    this->here = spots.back()->here;
  }

  template <typename SpotType, typename... ArgTypes>
  void add_spot(ArgTypes&&... args) {
    assert(!spots.empty());
    spots.emplace_back(
        new SpotType(*spots.back(), std::forward<ArgTypes>(args)...));
    this->here = spots.back()->here;
  }

  const spot<ObjectType>& back(size_t count = 0) const noexcept {
    assert(count < spots.size());
    return **(spots.end() - count - 1);
  }

  const spot<ObjectType>& front(size_t count = 0) const noexcept {
    assert(count < spots.size());
    return **(spots.begin() + count);
  }

  spot<ObjectType>& back(size_t count = 0) noexcept {
    return const_cast<spot<ObjectType>&>(
        const_cast<const path<ObjectType>*>(this)->back(count));
  }

  spot<ObjectType>& front(size_t count = 0) noexcept {
    return const_cast<spot<ObjectType>&>(
        const_cast<const path<ObjectType>*>(this)->front(count));
  }

  void clear() noexcept { spots.clear(); }

  template <typename RootSpot = spot<ObjectType>, typename... ArgTypes>
  void reset_root(ArgTypes&&... args) {
    spots.clear();
    spots.emplace_back(new RootSpot(std::forward<ArgTypes>(args)...));
    this->here = spots.back()->here;
  }

 private:
  std::vector<std::unique_ptr<spot<ObjectType>>> spots;
};

}  // namespace cow
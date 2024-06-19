#pragma once

#include "cow/ptr.h"

namespace cow {

template <typename ObjectType>
class spot {
 public:
  using object_type = ObjectType;

  const ptr<ObjectType>* here;
  const ObjectType* get() const noexcept {
    return here ? here->get() : nullptr;
  }

  const ObjectType* operator->() const noexcept {
    assert(here && *here);
    return here->get();
  }

  ObjectType* operator--(int) { return write(); }
  ObjectType* operator--() { return write(); }

  virtual ObjectType* write() {
    return here ? --*const_cast<ptr<ObjectType>*>(here) : nullptr;
  }

  virtual const std::type_info& type_info() const noexcept {
    return typeid(object_type);
  }

  explicit operator bool() const noexcept { return here && *here; }

  size_t use_count() const noexcept { return here ? here->use_count() : 0; }

  template <typename StepFunc>
  auto step(StepFunc&& func) noexcept;

  spot(ptr<ObjectType>* where = nullptr) noexcept : here(where) {}

  spot(const spot&) = delete;
  spot(spot&& other) noexcept : here(other) { other.here = nullptr; }
  spot& operator=(const spot&) = delete;
  spot& operator=(spot&& other) noexcept {
    if (this != &other) {
      here = other.here;
      other.here = nullptr;
    }
    return *this;
  }
};

template <typename FromObjectType, typename StepFunc, typename ToObjectType>
class next_spot final : public spot<ToObjectType> {
 public:
  spot<FromObjectType>* fromSpot;
  const FromObjectType* fromObject;
  StepFunc stepFunc;

  next_spot(spot<FromObjectType>& from, StepFunc&& stepFunc) noexcept
      : fromSpot(&from),
        fromObject(from.get()),
        stepFunc(std::forward<StepFunc>(stepFunc)) {
    if (fromObject) {
      this->here = stepFunc(*fromObject);
    }
  }

  ToObjectType* write() override {
    if (fromSpot == nullptr) {
      return nullptr;
    }

    auto newObject = fromSpot->write();

    if (newObject == nullptr) {
      fromObject = nullptr;
      this->here = nullptr;
      return nullptr;
    }

    if (newObject != fromObject) {
      fromObject = newObject;
      this->here = stepFunc(*newObject);
    }

    return spot<ToObjectType>::write();
  }

  next_spot(const next_spot&) = delete;
  next_spot(next_spot&& other) noexcept
      : spot<ToObjectType>(std::move(other)),
        fromSpot(other.fromSpot),
        fromObject(other.fromObject),
        stepFunc(std::move(other.stepFunc)) {
    other.fromSpot = nullptr;
    other.fromObject = nullptr;
  }
  next_spot& operator=(const next_spot&) = delete;
  next_spot& operator=(next_spot&& other) noexcept {
    if (this != &other) {
      this->here = other.here;
      other.here = nullptr;

      fromSpot = other.fromSpot;
      other.fromSpot = nullptr;
      
      fromObject = other.fromObject;
      other.fromObject = nullptr;
      
      stepFunc = std::move(other.stepFunc);
    }
    return *this;
  }
};

template <typename FromObjectType>
template <typename StepFunc>
auto spot<FromObjectType>::step(StepFunc&& func) noexcept {
  using ToCowPtrType = std::remove_const_t<
      std::remove_pointer_t<decltype(func(*(const FromObjectType*)nullptr))>>;
  using ToObjectType = typename ToCowPtrType::object_type;
  return next_spot<FromObjectType, StepFunc, ToObjectType>(
      *this, std::forward<StepFunc>(func));
}


class path {
 public:

};

}  // namespace cow
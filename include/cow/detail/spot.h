#pragma once

#include "cow/spot.h"

namespace cow {

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// spot
//

template <typename ObjectType>
inline spot<ObjectType>::spot(spot&& other) noexcept : here(other.here) {
  other.here = nullptr;
}

template <typename ObjectType>
inline spot<ObjectType>& spot<ObjectType>::operator=(spot&& other) noexcept {
  assert(this != &other);  // virtual class; derived must check this
  here = other.here;
  other.here = nullptr;
  return *this;
}

template <typename ObjectType>
inline spot<ObjectType>::operator bool() const noexcept {
  return here && *here;
}

template <typename ObjectType>
inline const ObjectType& spot<ObjectType>::operator*() const noexcept {
  assert(here);
  return **here;
}

template <typename ObjectType>
inline const ObjectType* spot<ObjectType>::get() const noexcept {
  return here ? here->get() : nullptr;
}

template <typename ObjectType>
inline const ObjectType* spot<ObjectType>::operator->() const noexcept {
  assert(here && *here);
  return here->get();
}

template <typename ObjectType>
inline ObjectType* spot<ObjectType>::operator--(int) noexcept {
  return write();
}

template <typename ObjectType>
inline ObjectType* spot<ObjectType>::operator--() noexcept {
  return write();
}

template <typename ObjectType>
inline const std::type_info& spot<ObjectType>::type_info() const noexcept {
  return typeid(object_type);
}

template <typename ObjectType>
inline size_t spot<ObjectType>::use_count() const noexcept {
  return here ? here->use_count() : 0;
}

template <typename FromObjectType>
template <typename StepFunc>
auto spot<FromObjectType>::step(StepFunc&& func) noexcept {
  using ToCowPtrType = std::remove_const_t<
      std::remove_pointer_t<decltype(func(*(const FromObjectType*)nullptr))>>;
  using ToObjectType = typename ToCowPtrType::object_type;
  return lambda_spot<FromObjectType, ToObjectType, StepFunc>(
      *this, std::forward<StepFunc>(func));
}

template <typename FromObjectType>
template <typename ToObjectType>
offset_spot<FromObjectType, ToObjectType> spot<FromObjectType>::step(
    const ptr<ToObjectType>* pointerToPointerInFromObject) noexcept {
  return offset_spot<FromObjectType, ToObjectType>(
      *this, pointerToPointerInFromObject);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// root_spot
//

template <typename ObjectType>
inline root_spot<ObjectType>::root_spot() noexcept : spot(nullptr) {}

template <typename ObjectType>
inline root_spot<ObjectType>::root_spot(ptr<ObjectType>* where) noexcept
    : spot<ObjectType>(where) {}

template <typename ObjectType>
inline root_spot<ObjectType>& root_spot<ObjectType>::operator=(
    root_spot&& other) noexcept {
  if (this != other) {
    spot<ObjectType>::operator=(std::move(other));
  }
}

template <typename ObjectType>
inline ObjectType* root_spot<ObjectType>::write() noexcept {
  return this->here ? --*const_cast<ptr<ObjectType>*>(this->here) : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// next_spot
//

template <typename FromObjectType, typename ToObjectType>
inline ToObjectType* next_spot<FromObjectType, ToObjectType>::write() noexcept {
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

template <typename FromObjectType, typename ToObjectType>
inline next_spot<FromObjectType, ToObjectType>::next_spot(
    spot<FromObjectType>& from,
    const ptr<ToObjectType>* where) noexcept
    : spot<ToObjectType>(where), fromSpot(&from), hasWritten(false) {}

template <typename FromObjectType, typename ToObjectType>
inline next_spot<FromObjectType, ToObjectType>::next_spot(
    next_spot&& other) noexcept
    : spot<ToObjectType>(std::move(other)),
      fromSpot(other.fromSpot),
      hasWritten(other.hasWritten) {
  other.fromSpot = nullptr;
  other.hasWritten = false;
}

template <typename FromObjectType, typename ToObjectType>
inline next_spot<FromObjectType, ToObjectType>&
next_spot<FromObjectType, ToObjectType>::operator=(next_spot&& other) noexcept {
  spot<ToObjectType>::operator=(std::move(other));

  fromSpot = other.fromSpot;
  other.fromSpot = nullptr;

  hasWritten = other.hasWritten;
  other.hasWritten = false;

  return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// lambda_spot
//

template <typename FromObjectType, typename ToObjectType, typename StepFunc>
inline lambda_spot<FromObjectType, ToObjectType, StepFunc>::lambda_spot(
    spot<FromObjectType>& from,
    StepFunc&& stepFunc) noexcept
    : next_spot<FromObjectType, ToObjectType>(from,
                                              from ? stepFunc(*from) : nullptr),
      stepFunc(std::forward<StepFunc>(stepFunc)) {}

template <typename FromObjectType, typename ToObjectType, typename StepFunc>
inline lambda_spot<FromObjectType, ToObjectType, StepFunc>&
lambda_spot<FromObjectType, ToObjectType, StepFunc>::operator=(
    lambda_spot&& other) noexcept {
  if (this != &other) {
    next_spot::operator=(std::move(other));
    stepFunc = std::move(other.stepFunc);
  }
  return *this;
}

template <typename FromObjectType, typename ToObjectType, typename StepFunc>
inline const ptr<ToObjectType>*
lambda_spot<FromObjectType, ToObjectType, StepFunc>::step(
    const FromObjectType& from) const noexcept {
  return stepFunc(from);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// offset_spot
//

template <typename FromObjectType, typename ToObjectType>
inline offset_spot<FromObjectType, ToObjectType>::offset_spot(
    spot<FromObjectType>& from,
    const ptr<ToObjectType>* pointerToPointerWithinFromObject) noexcept
    : next_spot<FromObjectType, ToObjectType>(from,
                                              pointerToPointerWithinFromObject),
      offset(reinterpret_cast<const char*>(pointerToPointerWithinFromObject) -
             reinterpret_cast<const char*>(from.get())) {
  assert(from);
  assert(pointerToPointerWithinFromObject != nullptr);
  assert(offset >= 0 && offset < sizeof(FromObjectType) &&
         (offset & (alignof(cow::ptr<ToObjectType>) - 1)) == 0);
}

template <typename FromObjectType, typename ToObjectType>
inline const ptr<ToObjectType>* offset_spot<FromObjectType, ToObjectType>::step(
    const FromObjectType& from) const noexcept {
  return reinterpret_cast<const ptr<ToObjectType>*>(
      reinterpret_cast<const char*>(&from) + offset);
}

template <typename FromObjectType, typename ToObjectType>
inline offset_spot<FromObjectType, ToObjectType>&
offset_spot<FromObjectType, ToObjectType>::operator=(
    offset_spot&& other) noexcept {
  if (this != other) {
    next_spot::operator=(std::move(other));
    offset = other.offset;
  }
}

}  // namespace cow

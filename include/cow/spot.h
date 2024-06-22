#pragma once

#include "cow/ptr.h"

namespace cow {

template <typename FromObjectType, typename ToObjectType>
class offset_spot;

template <typename ObjectType>
class path;

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// spot
//

template <typename ObjectType>
class spot {
 public:
  using object_type = ObjectType;

  explicit operator bool() const noexcept;
  const ObjectType* get() const noexcept;
  const ObjectType& operator*() const noexcept;
  const ObjectType* operator->() const noexcept;

  virtual ObjectType* write() noexcept = 0;
  ObjectType* operator--(int) noexcept;
  ObjectType* operator--() noexcept;

  virtual const std::type_info& type_info() const noexcept;

  size_t use_count() const noexcept;

  template <typename StepFunc>
  auto step(StepFunc&& func) noexcept;

  template <typename ToObjectType>
  offset_spot<ObjectType, ToObjectType> step(
      const ptr<ToObjectType>* pointerToPointerInFromObject) noexcept;

 protected:
  friend class path<ObjectType>;

  explicit spot(const ptr<ObjectType>* where) : here(where) {}
  spot(const spot&) = delete;
  spot& operator=(const spot&) = delete;
  spot(spot&& other) noexcept;
  spot& operator=(spot&& other) noexcept;

  const ptr<ObjectType>* here{nullptr};
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// root_spot
//

template <typename ObjectType>
class root_spot final : public spot<ObjectType> {
 public:
  root_spot() noexcept;
  explicit root_spot(ptr<ObjectType>* where) noexcept;

  root_spot(root_spot&& other) noexcept = default;
  root_spot& operator=(root_spot&& other) noexcept;

  virtual ObjectType* write() noexcept override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// next_spot
//

template <typename FromObjectType, typename ToObjectType>
class next_spot : public spot<ToObjectType> {
 public:
  ToObjectType* write() noexcept final override;

 protected:
  next_spot(spot<FromObjectType>& from,
            const ptr<ToObjectType>* where) noexcept;

  next_spot(next_spot&& other) noexcept;

  next_spot& operator=(next_spot&& other) noexcept;

  virtual const ptr<ToObjectType>* step(
      const FromObjectType& from) const noexcept = 0;

 private:
  spot<FromObjectType>* fromSpot;
  bool hasWritten;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// lambda_spot
//

template <typename FromObjectType, typename ToObjectType, typename StepFunc>
class lambda_spot final : public next_spot<FromObjectType, ToObjectType> {
 public:
  lambda_spot(spot<FromObjectType>& from, StepFunc&& stepFunc) noexcept;

  lambda_spot(lambda_spot&& other) noexcept = default;
  lambda_spot& operator=(lambda_spot&& other) noexcept;

 protected:
  const ptr<ToObjectType>* step(
      const FromObjectType& from) const noexcept override;

 private:
  StepFunc stepFunc;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// offset_spot
//

template <typename FromObjectType, typename ToObjectType>
class offset_spot final : public next_spot<FromObjectType, ToObjectType> {
 public:
  offset_spot(
      spot<FromObjectType>& from,
      const ptr<ToObjectType>* pointerToPointerWithinFromObject) noexcept;

  const ptr<ToObjectType>* step(
      const FromObjectType& from) const noexcept override;

  offset_spot(offset_spot&& other) noexcept = default;
  offset_spot& operator=(offset_spot&& other) noexcept;

 private:
  size_t offset;
};

}  // namespace cow

#include "cow/detail/spot.h"

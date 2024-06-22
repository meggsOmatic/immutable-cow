#pragma once

#include "cow/spot.h"

#include <memory>
#include <vector>

namespace cow {

// TODO: Write a good scratch allocator and use it here :-)
template <typename ObjectType>
class path : public spot<ObjectType> {
 public:
  path() = default;

  // Construct with a root_spot
  explicit path(ptr<ObjectType>* rootWhere);

  // Construct with a root of the specified type
  template <typename RootSpotType = root_spot<ObjectType>, typename... ArgTypes>
  path create_root(ArgTypes&&... args);

  size_t size() const noexcept;

  ObjectType* write() noexcept override;

  // Remove from the back. pop() is the number to remove counting
  // from the back, and resize() is the number to keep counting
  // from the front, so pop(n) == resize(size() - n)
  // and resize(n) == pop(size() - n).
  path& pop(size_t countToRemove = 1) noexcept;
  path& resize(size_t newSizeAfterRemove) noexcept;

  template <typename StepFunc>
  path& push(StepFunc&& stepFunc) noexcept;

  path& push(const ptr<ObjectType>* pointerInBackObject) noexcept;

  template <typename SpotType, typename... ArgTypes>
  path& emplace(ArgTypes&&... args);

  const spot<ObjectType>& back(size_t count = 0) const noexcept;
  const spot<ObjectType>& front(size_t count = 0) const noexcept;
  spot<ObjectType>& back(size_t count = 0) noexcept;
  spot<ObjectType>& front(size_t count = 0) noexcept;

  path& clear() noexcept;

  template <typename RootSpot = spot<ObjectType>, typename... ArgTypes>
  path& reset(ArgTypes&&... args);

 private:
  std::vector<std::unique_ptr<spot<ObjectType>>> spots;
};

}  // namespace cow

#include "cow/detail/path.h"

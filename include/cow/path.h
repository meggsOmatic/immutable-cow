#pragma once

#include "cow/spot.h"

#include <memory>
#include <vector>

namespace cow {

template <typename FromObjectType, typename ToObjectType>
class offset_spot;

template <typename ObjectType>
class path;

// TODO: Write a good scratch allocator and use it here :-)
template <typename ObjectType>
class path : public spot<ObjectType> {
 public:
  path() {}

  explicit path(ptr<ObjectType>* rootWhere) : spot<ObjectType>(rootWhere) {
    spots.emplace_back(new root_spot<ObjectType>(rootWhere));
  }

  template <typename RootSpotType = root_spot<ObjectType>, typename... ArgTypes>
  path create(ArgTypes&&... args) {
    path<ObjectType> result;
    result.spots.emplace_back(new RootSpotType(std::forward<ArgTypes>(args)...));
    result.here = spots.back()->here;
    return result;
  }

  size_t size() const noexcept { return spots.size(); }

  ObjectType* write() noexcept override {
    ObjectType* result = nullptr;
    if (!spots.empty()) {
      spot<ObjectType>* back = spots.back().get();
      result = back->write();
      this->here = back->here;
    }
    return result;
  }

  path& pop(size_t count = 1) noexcept {
    size_t oldSize = spots.size();
    if (count < oldSize) {
      spots.resize(spots.size() - count);
      this->here = spots.back()->here;
    } else {
      spots.clear();
      this->here = nullptr;
    }
    return *this;
  }

  path& resize(size_t newSize) noexcept {
    if (newSize < spots.size()) {
      spots.resize(newSize);
      this->here = newSize > 0 ? spots.back()->here : nullptr;
    }
    return *this;
  }

  template <typename StepFunc>
  path& push(StepFunc&& stepFunc) noexcept {
    return emplace<lambda_spot<ObjectType, ObjectType, StepFunc>>(
        std::forward<StepFunc>(stepFunc));
  }

  path& push(const ptr<ObjectType>* pointerInBackObject) noexcept {
    return emplace<offset_spot<ObjectType, ObjectType>>(
        pointerInBackObject);
  }

  template <typename SpotType, typename... ArgTypes>
  path& emplace(ArgTypes&&... args) {
    assert(!spots.empty());
    SpotType* newSpot = new SpotType(*spots.back(), std::forward<ArgTypes>(args)...);
    this->here = newSpot->here;
    spots.emplace_back(newSpot);
    return *this;
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
    const auto* const_this = this;
    return const_cast<spot<ObjectType>&>(const_this->back(count));
  }

  spot<ObjectType>& front(size_t count = 0) noexcept {
    const auto* const_this = this;
    return const_cast<spot<ObjectType>&>(const_this->front(count));
  }

  path& clear() noexcept {
    spots.clear();
    this->here = nullptr;
    return *this;
  }

  template <typename RootSpot = spot<ObjectType>, typename... ArgTypes>
  path& reset(ArgTypes&&... args) {
    spots.clear();
    auto newRoot = new RootSpot(std::forward<ArgTypes>(args)...);
    this->here = newRoot->here;
    spots.emplace_back(newRoot);
    return *this;
  }

 private:
  std::vector<std::unique_ptr<spot<ObjectType>>> spots;
};

template<typename ObjectType, typename... StepLambdaType>
path<ObjectType> make_path(ptr<ObjectType>* root, StepLambdaType&&... stepFunc) {
  path<ObjectType> result(root);
  (result.push(std::forward<StepLambdaType>(stepFunc)), ...);
  return result;
}

}  // namespace cow
#pragma once

#include "cow/path.h"

namespace cow {

template <typename ObjectType, typename... StepLambdaType>
path<ObjectType> make_path(ptr<ObjectType>* root,
                           StepLambdaType&&... stepFunc) {
  path<ObjectType> result(root);
  (result.push(std::forward<StepLambdaType>(stepFunc)), ...);
  return result;
}

template <typename ObjectType>
template <typename RootSpotType, typename... ArgTypes>
inline path<ObjectType> path<ObjectType>::create_root(ArgTypes&&... args) {
  path<ObjectType> result;
  result.spots.emplace_back(new RootSpotType(std::forward<ArgTypes>(args)...));
  result.here = spots.back()->here;
  return result;
}

template <typename ObjectType>
inline path<ObjectType>::path(ptr<ObjectType>* rootWhere)
    : spot<ObjectType>(rootWhere) {
  spots.emplace_back(new root_spot<ObjectType>(rootWhere));
}

template <typename ObjectType>
inline size_t path<ObjectType>::size() const noexcept {
  return spots.size();
}

template <typename ObjectType>
inline ObjectType* path<ObjectType>::write() noexcept {
  ObjectType* result = nullptr;
  if (!spots.empty()) {
    spot<ObjectType>* back = spots.back().get();
    result = back->write();
    this->here = back->here;
  }
  return result;
}

template <typename ObjectType>
inline path<ObjectType>& path<ObjectType>::pop(size_t count) noexcept {
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

template <typename ObjectType>
inline path<ObjectType>& path<ObjectType>::resize(size_t newSize) noexcept {
  if (newSize < spots.size()) {
    spots.resize(newSize);
    this->here = newSize > 0 ? spots.back()->here : nullptr;
  }
  return *this;
}

template <typename ObjectType>
inline path<ObjectType>& path<ObjectType>::push(
    const ptr<ObjectType>* pointerInBackObject) noexcept {
  return emplace<offset_spot<ObjectType, ObjectType>>(pointerInBackObject);
}

template <typename ObjectType>
inline const spot<ObjectType>& path<ObjectType>::back(
    size_t count) const noexcept {
  assert(count < spots.size());
  return **(spots.end() - count - 1);
}

template <typename ObjectType>
inline const spot<ObjectType>& path<ObjectType>::front(
    size_t count) const noexcept {
  assert(count < spots.size());
  return **(spots.begin() + count);
}

template <typename ObjectType>
inline spot<ObjectType>& path<ObjectType>::back(size_t count) noexcept {
  const auto* const_this = this;
  return const_cast<spot<ObjectType>&>(const_this->back(count));
}

template <typename ObjectType>
inline spot<ObjectType>& path<ObjectType>::front(size_t count) noexcept {
  const auto* const_this = this;
  return const_cast<spot<ObjectType>&>(const_this->front(count));
}

template <typename ObjectType>
inline path<ObjectType>& path<ObjectType>::clear() noexcept {
  spots.clear();
  this->here = nullptr;
  return *this;
}

template <typename ObjectType>
template <typename StepFunc>
inline path<ObjectType>& path<ObjectType>::push(StepFunc&& stepFunc) noexcept {
  return emplace<lambda_spot<ObjectType, ObjectType, StepFunc>>(
      std::forward<StepFunc>(stepFunc));
}

template <typename ObjectType>
template <typename SpotType, typename... ArgTypes>
inline path<ObjectType>& path<ObjectType>::emplace(ArgTypes&&... args) {
  assert(!spots.empty());
  SpotType* newSpot =
      new SpotType(*spots.back(), std::forward<ArgTypes>(args)...);
  this->here = newSpot->here;
  spots.emplace_back(newSpot);
  return *this;
}

template <typename ObjectType>
template <typename RootSpot, typename... ArgTypes>
inline path<ObjectType>& path<ObjectType>::reset(ArgTypes&&... args) {
  spots.clear();
  auto newRoot = new RootSpot(std::forward<ArgTypes>(args)...);
  this->here = newRoot->here;
  spots.emplace_back(newRoot);
  return *this;
}

}  // namespace cow
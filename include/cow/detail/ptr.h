#pragma once

#include "cow/ptr.h"
#include "cow/detail/control_block.h"

#include <assert.h>

namespace cow {
  template<typename ObjectType>
  inline detail::control_block_with_object<ObjectType>* ptr<ObjectType>::control() const noexcept {
    assert(object);
    return reinterpret_cast<detail::control_block_with_object<ObjectType>*>(
      reinterpret_cast<char*>(object) -
      offsetof(detail::control_block_with_object<ObjectType>, object));
  }

  template<typename ObjectType>
  inline ptr<ObjectType>::ptr(std::nullptr_t) : object(nullptr) {}
  
  template<typename ObjectType>
  inline ptr<ObjectType>::ptr(const ptr& other) : object(other.object) {
    if (object) {
      control()->incRef();
    }
  }

  template<typename ObjectType>
  inline ptr<ObjectType>::ptr(ptr&& other) : object(other.object) {
    other.object = nullptr;
  }

  template<typename ObjectType>
  inline ptr<ObjectType>& ptr<ObjectType>::operator=(const ptr& other) {
    if (other.object) {
      other.control()->incRef();
    }
    if (object) {
      control()->decRef();
    }
    object = other.object;
    return *this;
  }

  template<typename ObjectType>
  inline ptr<ObjectType>& ptr<ObjectType>::operator=(ptr&& other) {
    if (this != &other) {
      if (object) {
        control()->decRef();
      }
      object = other.object;
      other.object = nullptr;
    }
    return *this;
  }

  template<typename ObjectType>
  inline ptr<ObjectType>::~ptr() {
    if (object) {
      control()->decRef();
    }
  }

  template<typename ObjectType>
  inline ptr<ObjectType>::operator bool() const noexcept {
    return object != nullptr;
  }

  template<typename ObjectType>
  inline const ObjectType* ptr<ObjectType>::operator->() const noexcept {
    assert(object);
    return object;
  }

  template<typename ObjectType>
  inline const ObjectType& ptr<ObjectType>::operator*() const noexcept {
    assert(object);
    return *object;
  }

  template<typename ObjectType>
  inline ObjectType* ptr<ObjectType>::operator--(int) noexcept {
    if (object) {
      auto* c = control();
      if (c->refCount.load(std::memory_order_relaxed) > 1) {
        auto* clone = c->clone();
        assert(clone && clone->refCount.load(std::memory_order_relaxed) == 1);
        c->decRef();
        object = &clone->object;
      }
    }
    return object;
  }

  template<typename ObjectType>
  inline ObjectType* ptr<ObjectType>::operator--() noexcept {
    return operator--(0);
  }

  template<typename ObjectType>
  inline const ObjectType* ptr<ObjectType>::get() const noexcept {
    return object;
  }

  template<typename ObjectType>
  inline size_t ptr<ObjectType>::use_count() const noexcept {
    return object ? control()->refCount.load(std::memory_order_relaxed) : 0;
  }

  template<typename ObjectType>
  inline ptr<ObjectType>::ptr(ObjectType* objectPtr) noexcept : object(objectPtr) {
    assert(!object || control()->refCount.load(std::memory_order_relaxed) == 1);
  }

  template<typename ObjectType, typename... ObjectContructorArgTypes>
  inline ptr<ObjectType> make(ObjectContructorArgTypes&&... objectConstructorArgs) {
    auto* const control_block = new detail::control_block_with_object<ObjectType>(std::forward<ObjectContructorArgTypes>(objectConstructorArgs)...);
    ObjectType* object = &control_block->object;
    return ptr<ObjectType>(object);
  }

  template<typename ObjectType1, typename ObjectType2>
  inline bool operator==(const ptr<ObjectType1>& ptr1, const ptr<ObjectType2>& ptr2) {
    return ptr1.object == ptr2.object;
  }

  template<typename ObjectType1, typename ObjectType2>
  inline bool operator==(const ptr<ObjectType1>& ptr1, const ObjectType2* ptr2) {
    return ptr1.object == ptr2;
  }

  template<typename ObjectType1, typename ObjectType2>
  inline bool operator==(const ObjectType1* ptr1, const ptr<ObjectType2>& ptr2) {
    return ptr1 == ptr2.object;
  }

  template<typename ObjectType1>
  inline bool operator==(const ptr<ObjectType1>& ptr1, std::nullptr_t) {
    return ptr1.object == nullptr;
  }

  template<typename ObjectType2>
  inline bool operator==(std::nullptr_t, const ptr<ObjectType2>& ptr2) {
    return nullptr == ptr2.object;
  }
}


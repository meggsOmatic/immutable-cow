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
  inline ptr<ObjectType>::ptr(const ptr& other) noexcept : object(other.object) {
    if (object) {
      control()->incRef();
    }
  }

  template<typename ObjectType>
  inline ptr<ObjectType>::ptr(ptr&& other) noexcept : object(other.object) {
    other.object = nullptr;
  }

  template <typename ObjectType>
  template <
      typename DerivedType,
      std::enable_if_t<std::is_convertible_v<DerivedType*, ObjectType*>, int>>
  inline ptr<ObjectType>::ptr(const ptr<DerivedType>& other) noexcept
      : object(other.object) {
    assert((void*)object == (void*)other.object); // control blocks must match
    if (object) {
      control()->incRef();
    }
  }

  template <typename ObjectType>
  template <
      typename DerivedType,
      std::enable_if_t<std::is_convertible_v<DerivedType*, ObjectType*>, int>>
  inline ptr<ObjectType>::ptr(ptr<DerivedType>&& other) noexcept : object(other.object) {
    assert((void*)object == (void*)other.object); // control blocks must match
    other.object = nullptr;
  }

  template <typename ObjectType>
  inline ptr<ObjectType>& ptr<ObjectType>::operator=(const ptr& other) noexcept {
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
  inline ptr<ObjectType>& ptr<ObjectType>::operator=(ptr&& other) noexcept {
    if (this != &other) {
      if (object) {
        control()->decRef();
      }
      object = other.object;
      other.object = nullptr;
    }
    return *this;
  }

  template <typename ObjectType>
  template <
      typename DerivedType,
      std::enable_if_t<std::is_convertible_v<DerivedType*, ObjectType*>, int>>
  inline ptr<ObjectType>& ptr<ObjectType>::operator=(
      const ptr<DerivedType>& other) noexcept {
    if (other.object) {
      other.control()->incRef();
    }
    if (object) {
      control()->decRef();
    }
    object = other.object;
    return *this;
  }

  template <typename ObjectType>
  template <typename DerivedType,
            std::enable_if_t<std::is_convertible_v<DerivedType*, ObjectType*>, int>>
  inline ptr<ObjectType>& ptr<ObjectType>::operator=(ptr<DerivedType>&& other) noexcept {
    if (object) {
      control()->decRef();
    }
    object = other.object;
    other.object = nullptr;
    return *this;
  }

  template <typename ObjectType>
  inline ptr<ObjectType>& ptr<ObjectType>::operator=(std::nullptr_t) noexcept {
    if (object) {
      control()->decRef();
      object = nullptr;
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

  template <typename ObjectType>
  inline void ptr<ObjectType>::reset() noexcept {
    if (object) {
      control()->decRef();
      object = nullptr;
    }
  }

  template<typename ObjectType>
  inline const ObjectType& ptr<ObjectType>::operator*() const noexcept {
    assert(object);
    return *object;
  }

  template <typename ObjectType>
  inline const ObjectType* ptr<ObjectType>::operator->() const noexcept {
    assert(object);
    return object;
  }

  template <typename ObjectType>
  inline const ObjectType* ptr<ObjectType>::get() const noexcept {
    return object;
  }

  template <typename ObjectType>
  inline const ObjectType* ptr<ObjectType>::read() const noexcept {
    return object;
  }

  template <typename ObjectType>
  inline ObjectType* ptr<ObjectType>::operator--(int) noexcept {
    return write();
  }

  template<typename ObjectType>
  inline ObjectType* ptr<ObjectType>::operator--() noexcept {
    return write();
  }

  template <typename ObjectType>
  inline ObjectType* ptr<ObjectType>::write() noexcept {
    if (object) {
      auto* c = control();
      if (c->refCount.load(std::memory_order_acquire) > 1) {
        auto* clone = c->clone();
        assert(clone && clone->refCount.load(std::memory_order_relaxed) == 1);
        c->decRef();
        object = &clone->object;
      }
    }
    return object;
  }

  template <typename ObjectType>
  template <typename StaticCastType>
  inline const StaticCastType* ptr<ObjectType>::read() const noexcept {
    if constexpr (std::is_polymorphic_v<StaticCastType>) {
      assert(dynamic_cast<StaticCastType*>(object) ==
             static_cast<StaticCastType*>(object));
    }
    return static_cast<StaticCastType*>(object);
  }

  template <typename ObjectType>
  template <typename DynamicCastType>
  inline const DynamicCastType* ptr<ObjectType>::read_if() const noexcept {
    return dynamic_cast<DynamicCastType*>(object);
  }

  template <typename ObjectType>
  template <typename StaticCastType>
  inline StaticCastType* ptr<ObjectType>::write() noexcept {
    if constexpr (std::is_polymorphic_v<StaticCastType>) {
      assert(dynamic_cast<StaticCastType*>(object) ==
             static_cast<StaticCastType*>(object));
    }
    return static_cast<StaticCastType*>(write());
  }

  template <typename ObjectType>
  template <typename DynamicCastType>
  inline DynamicCastType* ptr<ObjectType>::write_if() noexcept {
    return dynamic_cast<DynamicCastType*>(write());
  }

  template <typename ObjectType>
  template <typename StaticCastType>
  inline ptr<StaticCastType> ptr<ObjectType>::cast() const noexcept {
    if constexpr (std::is_polymorphic_v<StaticCastType>) {
      assert((void*)dynamic_cast<StaticCastType*>(object) == (void*)object);
    }
    return ptr<StaticCastType>(
        object ? (control()->incRef(), static_cast<StaticCastType*>(object))
               : nullptr);
  }

  template <typename ObjectType>
  template <typename StaticCastType>
  inline ptr<StaticCastType> ptr<ObjectType>::move_cast() noexcept {
    if constexpr (std::is_polymorphic_v<StaticCastType>) {
      assert((void*)dynamic_cast<StaticCastType*>(object) == (void*)object);
    }
    auto* p = object;
    object = nullptr;
    return ptr<StaticCastType>(static_cast<StaticCastType*>(p));
  }

  template <typename ObjectType>
  template <typename DynamicCastType>
  inline ptr<DynamicCastType> ptr<ObjectType>::dynamic() const noexcept {
    auto* const dyn = dynamic_cast<DynamicCastType*>(object);
    if (dyn) {
      control()->incRef();
    }
    return ptr<DynamicCastType>(dyn);
  }

  template <typename ObjectType>
  template <typename DynamicCastType>
  inline ptr<DynamicCastType> ptr<ObjectType>::move_dynamic() noexcept {
    auto* const dyn = dynamic_cast<DynamicCastType*>(object);
    if (object) {
      if (!dyn) {
        control()->decRef();
      } else {
        assert((void*)dyn == (void*)object);
      }
      object = nullptr;
    }
    return ptr<DynamicCastType>(dyn);
  }

  template <typename ObjectType>
  inline size_t ptr<ObjectType>::use_count() const noexcept {
    return object ? control()->refCount.load(std::memory_order_relaxed) : 0;
  }

  template <typename ObjectType>
  inline const std::type_info& ptr<ObjectType>::type_info() const noexcept {
    return object ? control()->type_info() : typeid(nullptr);
  }

  template <typename ObjectType>
  inline ptr<ObjectType>::ptr(ObjectType* objectPtr) noexcept : object(objectPtr) {
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

  template <typename DestType, typename SourceType>
  ptr<DestType> static_pointer_cast(const ptr<SourceType>& src) {
    return src.cast<DestType>();
  }
  
  template <typename DestType, typename SourceType>
  ptr<DestType> static_pointer_cast(ptr<SourceType>&& src) {
    return src.move_cast<DestType>();
  }
  
  template <typename DestType, typename SourceType>
  ptr<DestType> dynamic_pointer_cast(const ptr<SourceType>& src) {
    return src.dynamic<DestType>();
  }
  
  template <typename DestType, typename SourceType>
  ptr<DestType> dynamic_pointer_cast(ptr<SourceType>&& src) {
    return src.move_dynamic<DestType>();
  }
  }


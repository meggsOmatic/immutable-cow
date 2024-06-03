#pragma once

#include <typeinfo>

namespace cow {

  namespace detail {
    class control_block;
    template<typename ObjectType> class control_block_with_object;
  }

  template<typename ObjectType>
  class ptr final {
  public:
    using object_type = ObjectType;

    ptr(std::nullptr_t = nullptr);
    ~ptr();

    ptr(const ptr&) noexcept;
    ptr(ptr&&) noexcept;

    template <typename DerivedType,
              std::enable_if_t<std::is_convertible_v<DerivedType*, ObjectType*>,
                               int> = 0>
    ptr(const ptr<DerivedType>&) noexcept;

    template <typename DerivedType,
              std::enable_if_t<std::is_convertible_v<DerivedType*, ObjectType*>,
                               int> = 0>
    ptr(ptr<DerivedType>&&) noexcept;

    ptr& operator=(const ptr&) noexcept;
    ptr& operator=(ptr&&) noexcept;
    
    template <typename DerivedType,
              std::enable_if_t<std::is_convertible_v<DerivedType*, ObjectType*>,
                               int> = 0>
    ptr& operator=(const ptr<DerivedType>&) noexcept;
    
    template <typename DerivedType,
              std::enable_if_t<std::is_convertible_v<DerivedType*, ObjectType*>,
                               int> = 0>
    ptr& operator=(ptr<DerivedType>&&) noexcept;

    ptr& operator=(std::nullptr_t) noexcept;

    explicit operator bool() const noexcept;

    void reset() noexcept;

    const ObjectType& operator*() const noexcept;
    const ObjectType* operator->() const noexcept;
    const ObjectType* get() const noexcept;
    const ObjectType* read() const noexcept;

    ObjectType* operator--(int) noexcept;
    ObjectType* operator--() noexcept;
    ObjectType* write() noexcept;
    
    template<typename StaticCastType>
    const StaticCastType* read() const noexcept;

    template <typename DynamicCastType>
    const DynamicCastType* read_if() const noexcept;

    template <typename StaticCastType>
    StaticCastType* write() noexcept;

    template <typename DynamicCastType>
    DynamicCastType* write_if() noexcept;

    template <typename StaticCastType>
    ptr<StaticCastType> cast() const noexcept;

    template <typename StaticCastType>
    ptr<StaticCastType> move_cast() noexcept;

    template <typename DynamicCastType>
    ptr<DynamicCastType> dynamic() const noexcept;

    template <typename DynamicCastType>
    ptr<DynamicCastType> move_dynamic() noexcept;

    size_t use_count() const noexcept;
    
    // Returns the dynamic type of the pointed-to object, or typeid(nullptr)
    // if null. For static typing, using typeid(myptr::object_type) instead.
    const std::type_info& type_info() const noexcept;

  private:
    template<typename OtherType> friend class ptr;

    ObjectType* object;

    explicit ptr(ObjectType* objectPtr) noexcept;

    detail::control_block_with_object<ObjectType>* control() const noexcept;

    template<typename ObjectType, typename... ObjectContructorArgTypes>
    friend ptr<ObjectType> make(ObjectContructorArgTypes&&...);

    template<typename ObjectType1, typename ObjectType2>
    friend bool operator==(const ptr<ObjectType1>& ptr1, const ptr<ObjectType2>& ptr2);

    template<typename ObjectType1, typename ObjectType2>
    friend bool operator==(const ptr<ObjectType1>& ptr1, const ObjectType2* ptr2);

    template<typename ObjectType1, typename ObjectType2>
    friend bool operator==(const ObjectType1* ptr1, const ptr<ObjectType2>& ptr2);

    template<typename ObjectType1>
    friend bool operator==(const ptr<ObjectType1>& ptr1, std::nullptr_t); 

    template<typename ObjectType2>
    friend bool operator==(std::nullptr_t, const ptr<ObjectType2>& ptr2);
  };

  template<typename DestType, typename SourceType>
  inline ptr<DestType> static_pointer_cast(const ptr<SourceType>& src);

  template<typename DestType, typename SourceType>
  inline ptr<DestType> static_pointer_cast(ptr<SourceType>&& src);

  template <typename DestType, typename SourceType>
  inline ptr<DestType> dynamic_pointer_cast(const ptr<SourceType>& src);

  template <typename DestType, typename SourceType>
  inline ptr<DestType> dynamic_pointer_cast(ptr<SourceType>&& src);
}


#include "cow/detail/ptr.h"


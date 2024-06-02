#pragma once

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
    ptr& operator=(const ptr&) noexcept;
    ptr& operator=(ptr&&) noexcept;
    ptr& operator=(std::nullptr_t) noexcept;

    explicit operator bool() const noexcept;

    const ObjectType& operator*() const noexcept;
    const ObjectType* operator->() const noexcept;

    ObjectType* operator--(int) noexcept;
    ObjectType* operator--() noexcept;

    const ObjectType* get() const noexcept;

    size_t use_count() const noexcept;

  private:
    ObjectType* object;

    ptr(ObjectType* objectPtr) noexcept;

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
}

#include "cow/detail/ptr.h"


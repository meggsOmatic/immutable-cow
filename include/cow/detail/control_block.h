#pragma once

#include <atomic>
#include <assert.h>

namespace cow::detail {
  class control_block {
  public:
    std::atomic<size_t> refCount{ 1 };

    virtual ~control_block() {
      assert(refCount.load(std::memory_order_relaxed) == 0);
    }

    virtual control_block* clone() const noexcept = 0;

    inline void incRef() noexcept {
      auto postInc = refCount.fetch_add(1, std::memory_order_acquire);
      assert(postInc != 0);
    }

    inline void decRef() noexcept {
      auto postDec = refCount.fetch_sub(1, std::memory_order_acquire);
      assert((postDec + 1) != 0);
      if (postDec == 0) {
        delete this;
      }
    }

  protected:
    control_block() = default;
  };

  template<typename ObjectType>
  class control_block_with_object : public control_block {
  public:
    template<typename... ObjectContructorArgTypes>
    control_block_with_object(ObjectContructorArgTypes&&... objectConstructorArgs) :
      control_block(),
      object(std::forward<ObjectContructorArgTypes>(objectConstructorArgs)...) {}

    control_block_with_object* clone() const noexcept {
      return new control_block_with_object(object);
    }

    ObjectType object;
  };
}

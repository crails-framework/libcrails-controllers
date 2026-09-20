#pragma once
#include "flash.hpp"
#include <boost/asio/co_spawn.hpp>

namespace Crails
{
  class CoroutineController : public FlashController
  {
    typedef FlashController Super;
  public:
    using Super::Super;

  protected:
    virtual boost::asio::any_io_executor get_io_executor();

    template<typename TASK>
    void co_spawn(TASK task)
    {
      auto self = Super::shared_from_this();

      boost::asio::co_spawn
      (
        get_io_executor(),
        [self, task = std::move(task)]() mutable
        {
          return task();
        },
        [this, self](std::exception_ptr error)
        {
          if (error)
            context->protect([error]() { std::rethrow_exception(error); });
        }
      );
    }
  };
}

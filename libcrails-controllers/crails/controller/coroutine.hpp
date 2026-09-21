#pragma once
#include "flash.hpp"
#include <crails/context.hpp>
#include <boost/asio/co_spawn.hpp>

namespace Crails
{
  class CoroutineController : public FlashController
  {
    template<typename CONTROLLER>
    friend class ActionRoute;

    typedef FlashController Super;
  public:
    using Super::Super;

  protected:
    virtual boost::asio::any_io_executor get_io_executor();
    virtual boost::asio::awaitable<void> co_initialize() { co_return ; }
    virtual boost::asio::awaitable<void> co_finalize()   { co_return ; }

    template<typename TASK>
    void co_spawn(TASK task)
    {
      static_assert(
        std::is_same_v<std::invoke_result_t<TASK&>, boost::asio::awaitable<void>>,
        "Crails::CoroutineController::co_spawn: the task must be callable without arguments, and return a boost::asio::awaitable<void>"
      );
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

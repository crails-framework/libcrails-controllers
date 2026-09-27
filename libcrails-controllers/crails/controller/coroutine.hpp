#pragma once
#include "flash.hpp"
#include "coroutine/executor.hpp"
#include <crails/context.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/post.hpp>
#include <functional>
#include <thread>
#include <utility>

namespace Crails
{
  class CoroutineController : public FlashController
  {
    template<typename CONTROLLER, bool>
    friend class ActionRoute;

    typedef FlashController Super;
  public:
    CoroutineController(Context& context) :
      FlashController(context),
      coroutine_executor(std::make_shared<CoroutineExecutor>())
    {}

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

    template<typename T>
    boost::asio::awaitable<T> awaitable_blocking_task(std::function<T()> work)
    {
      auto executor = co_await boost::asio::this_coro::executor;

      co_return co_await boost::asio::async_initiate<decltype(boost::asio::use_awaitable), void(T)>(
        [work = std::move(work), executor](auto handler) mutable
        {
          std::thread([work = std::move(work), handler = std::move(handler), executor]() mutable
          {
            T result = work();

            boost::asio::post(executor, [handler = std::move(handler), result = std::move(result)]() mutable
            {
              std::move(handler)(std::move(result));
            });
          }).detach();
        },
        boost::asio::use_awaitable
      );
    }

    std::shared_ptr<CoroutineExecutor> coroutine_executor;
  };
}

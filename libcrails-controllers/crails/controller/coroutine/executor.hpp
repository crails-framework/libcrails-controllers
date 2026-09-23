#pragma once
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/defer.hpp>
#include <boost/asio/execution.hpp>
#include <boost/asio/query.hpp>
#include <boost/asio/require.hpp>
#include <boost/asio/prefer.hpp>
#include <functional>
#include <vector>
#include <memory>

namespace Crails
{
  struct CoroutineWrapper
  {
    std::function<void()> acquire;
    std::function<void()> release;
  };

  class CoroutineExecutor
  {
  public:
    struct ScopeGuard
    {
      std::shared_ptr<CoroutineExecutor> h;
      ScopeGuard(std::shared_ptr<CoroutineExecutor>&& hooks) : h(std::move(hooks))
      {
        if (h)
          h->on_acquire();
      }
      ~ScopeGuard()
      {
        if (h)
          h->on_release();
      }
    };

    void add_wrapper(const CoroutineWrapper& value)
    {
      wrappers.push_back(value);
    }

    void on_acquire() const
    {
      for (const auto& wrapper : wrappers)
      {
        if (wrapper.acquire)
          wrapper.acquire();
        return ;
      }
    }

    void on_release() const
    {
      for (const auto& wrapper : wrappers)
      {
        if (wrapper.release)
          wrapper.release();
        return ;
      }
    }

  private:
    std::vector<CoroutineWrapper> wrappers;
  };
  
class CoroutineBoundExecutor
  {
  public:
    typedef boost::asio::any_io_executor target_executor_type;

    CoroutineBoundExecutor(target_executor_type target, std::shared_ptr<CoroutineExecutor> executor_hooks)
      : target_(std::move(target)), hooks_(std::move(executor_hooks)) {}

    target_executor_type const& target() const noexcept { return target_; }

    bool operator==(const CoroutineBoundExecutor& other) const noexcept
    {
      return target_ == other.target_;
    }

    bool operator!=(const CoroutineBoundExecutor& other) const noexcept
    {
      return !(*this == other);
    }

    template <typename Property>
    auto query(const Property& p) const noexcept
      -> decltype(boost::asio::query(std::declval<const target_executor_type&>(), p))
    {
      return boost::asio::query(target_, p);
    }

    template <typename Property>
    auto require(const Property& p) const
      -> typename std::enable_if<
           boost::asio::can_require<target_executor_type, Property>::value,
           CoroutineBoundExecutor
         >::type
    {
      return CoroutineBoundExecutor(boost::asio::require(target_, p), hooks_);
    }

    template <typename Property>
    auto prefer(const Property& p) const
      -> typename std::enable_if<
           boost::asio::can_prefer<target_executor_type, Property>::value,
           CoroutineBoundExecutor
         >::type
    {
      return CoroutineBoundExecutor(boost::asio::prefer(target_, p), hooks_);
    }

    template <typename Function>
    void execute(Function&& f) const
    {
      boost::asio::execution::execute(target_, wrap_function(std::forward<Function>(f)));
    }

    template <typename Function, typename Alloc>
    void dispatch(Function&& f, const Alloc& a) const
    {
      boost::asio::dispatch(target_, wrap_function(std::forward<Function>(f)));
    }

    template <typename Function, typename Alloc>
    void post(Function&& f, const Alloc& a) const
    {
      boost::asio::post(target_, wrap_function(std::forward<Function>(f)));
    }

    template <typename Function, typename Alloc>
    void defer(Function&& f, const Alloc& a) const
    {
      boost::asio::defer(target_, wrap_function(std::forward<Function>(f)));
    }

  private:
    target_executor_type target_;
    std::shared_ptr<CoroutineExecutor> hooks_;

    template <typename Function>
    auto wrap_function(Function&& f) const
    {
      return [f = std::forward<Function>(f), hooks = hooks_]() mutable
      {
        CoroutineExecutor::ScopeGuard guard(std::move(hooks));

        std::move(f)();
      };
    }
  };

}

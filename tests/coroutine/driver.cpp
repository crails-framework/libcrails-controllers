#include <crails/environment.hpp>
#include <crails/controller/coroutine.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <thread>
#include "../test_support.hpp"

#undef NDEBUG
#include <cassert>

using namespace Crails;
using namespace Support;
using namespace std;
using boost::asio::awaitable;

static awaitable<void> pause(chrono::milliseconds duration)
{
  boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);

  timer.expires_after(duration);
  co_await timer.async_wait(boost::asio::use_awaitable);
}

struct Tasks : public Crails::CoroutineController
{
  Tasks(Crails::Context& context) : Crails::CoroutineController(context) {}
  template<typename TASK> void spawn(TASK task) { co_spawn(std::move(task)); }
  using Crails::CoroutineController::co_initialize;
  using Crails::CoroutineController::co_finalize;
  using Crails::CoroutineController::get_io_executor;
  int counter = 0;
};

struct OnTheIoContext : public Tasks
{
  OnTheIoContext(Crails::Context& context) : Tasks(context) {}
  boost::asio::any_io_executor get_io_executor() override { return Crails::Server::get_io_context().get_executor(); }
};

int main()
{
  Crails::environment = Crails::Test;
  Setup setup;

  // A task does not run before co_spawn returns: it runs on the strand of the connection, and it keeps the controller alive until it is over
  {
    Fixture fixture;
    auto    controller = make_shared<Tasks>(*fixture);
    weak_ptr<Tasks> weak = controller;
    bool started = false, on_strand = false, over = false;

    controller->spawn([&, raw = controller.get()]() -> awaitable<void>
    {
      started = true;
      on_strand = fixture.connection->get_strand().running_in_this_thread();
      co_await pause(10ms);
      on_strand = on_strand && fixture.connection->get_strand().running_in_this_thread();
      raw->counter = 42;
      over = true;
    });
    assert(!started);
    controller.reset();                  // nobody else holds it: the task does
    assert(!weak.expired());
    assert(pump_until([&]() { return over; }));
    assert(started && on_strand);
    assert(pump_until([&]() { return weak.expired(); }));     // released when the task is over
  }

  // The tasks of a controller never run at the same time: they can share its members without a lock
  {
    Fixture fixture;
    auto    controller = make_shared<Tasks>(*fixture);
    int     finished = 0;

    for (int i = 0 ; i < 2 ; ++i)
      controller->spawn([&, raw = controller.get()]() -> awaitable<void>
      {
        for (int step = 0 ; step < 20 ; ++step)
        {
          co_await pause(1ms);
          int value = raw->counter;
          this_thread::sleep_for(50us);
          raw->counter = value + 1;      // would lose updates if the tasks could run at the same time
        }
        ++finished;
      });
    assert(pump_until([&]() { return finished == 2; }));
    assert(controller->counter == 40);
  }

  // A task can start another one
  {
    Fixture fixture;
    auto    controller = make_shared<Tasks>(*fixture);
    bool    inner = false;

    controller->spawn([&, raw = controller.get()]() -> awaitable<void>
    {
      co_await pause(2ms);
      raw->spawn([&]() -> awaitable<void> { co_await pause(2ms); inner = true; });
    });
    assert(pump_until([&]() { return inner; }));
  }

  // A task that fails: the exception catcher answers
  {
    Fixture fixture;
    auto    controller = make_shared<Tasks>(*fixture);

    controller->spawn([]() -> awaitable<void> { co_await pause(2ms); throw std::runtime_error("the task failed"); });
    assert(pump_until([&]() { return fixture.finished(); }));
    assert(fixture.context->get_future().get() == 500);
  }

  // A task that fails, when a route runs the controller: the exception catcher answers, and the request is not closed (the callback, that runs the finalizers, is not called)
  {
    Fixture fixture;
    auto    controller = make_shared<Tasks>(*fixture);
    int     called = 0;

    Crails::ActionRoute<Tasks>::attach(*controller, [&]() { ++called; });
    controller->spawn([]() -> awaitable<void> { co_await pause(2ms); throw std::runtime_error("the task failed"); });
    assert(pump_until([&]() { return fixture.finished(); }));
    assert(fixture.context->get_future().get() == 500);
    assert(called == 0);
  }

  // The hooks do nothing by default, and do not wait: a coroutine that awaits them goes through at once
  {
    Fixture fixture;
    auto    controller = make_shared<Tasks>(*fixture);
    bool    done = false;

    controller->spawn([&, raw = controller.get()]() -> awaitable<void>
    {
      co_await raw->co_initialize();
      co_await raw->co_finalize();
      done = true;
    });
    assert(pump_until([&]() { return done; }));
  }

  // get_io_executor tells where the tasks run, and it can be changed
  {
    Fixture fixture;
    auto    controller = make_shared<OnTheIoContext>(*fixture);
    bool    over = false, on_strand = true;

    controller->spawn([&]() -> awaitable<void>
    {
      co_await pause(2ms);
      on_strand = fixture.connection->get_strand().running_in_this_thread();
      over = true;
    });
    assert(pump_until([&]() { return over; }));
    assert(!on_strand);
  }

  return 0;
}

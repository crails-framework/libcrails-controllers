#include <crails/environment.hpp>
#include "../test_support.hpp"

#undef NDEBUG
#include <cassert>

using namespace Crails;
using namespace Support;
using namespace std;

struct Bare : public Crails::ActionController
{
  Bare(Crails::Context& context) : Crails::ActionController(context) {}
  ~Bare() { if (on_destroy) on_destroy(); }
  function<void()> on_destroy;

  using Crails::ActionController::redirect_to;
  using Crails::ActionController::respond_with;
  using Crails::ActionController::close;
  using Crails::ActionController::initialize;
  using Crails::ActionController::finalize;
  using Crails::ActionController::params;
  using Crails::ActionController::response;
  using Crails::ActionController::is_closed;
};

typedef Crails::ActionRoute<Bare> Route;

int main()
{
  Crails::environment = Crails::Test;
  Setup setup;

  // A controller knows its name and its action: the router put them in the params
  {
    Fixture fixture;
    fixture.params()["controller-data"]["name"]   = "Things";
    fixture.params()["controller-data"]["action"] = "show";
    Bare controller(*fixture);

    assert(controller.get_controller_name() == "Things");
    assert(controller.get_action_name() == "show");
    assert(controller.get_vars().empty());
  }

  // redirect_to: 303 See Other unless a status is given, and the Location is what was given
  {
    Fixture fixture;
    Bare controller(*fixture);

    controller.redirect_to("/somewhere");
    assert(fixture.status() == 303 && fixture.header("Location") == "/somewhere");
  }
  {
    Fixture fixture;
    Bare controller(*fixture);

    controller.redirect_to(HttpStatus::moved_permanently, "/for-good");
    assert(fixture.status() == 301 && fixture.header("Location") == "/for-good");
  }

  // respond_with: a status, and nothing else
  {
    Fixture fixture;
    Bare controller(*fixture);

    controller.respond_with(HttpStatus::created);
    assert(fixture.status() == 201 && fixture.body().empty() && !fixture.has_header("Location"));
  }

  // Nothing runs the controller: closing it changes nothing, and nothing breaks
  {
    Fixture fixture;
    Bare controller(*fixture);

    controller.close();
    controller.close();
#ifdef PATCHED
    assert(!controller.is_closed());   // (nobody gave it a callback: there is nothing to close)
#endif
  }

  // Rendering, redirecting and responding are all ways to close the request
  {
    Fixture a, b;
    Bare redirects(*a), responds(*b);
    int called = 0;

    Route::attach(redirects, [&]() { ++called; });
    Route::attach(responds,  [&]() { ++called; });
    redirects.redirect_to("/x");
    assert(called == 1);
    responds.respond_with(HttpStatus::no_content);
    assert(called == 2);
  }

  // The time the controller took is recorded when it closes
  {
    Fixture fixture;
    Bare controller(*fixture);

    Route::attach(controller, []() {});
    assert(!fixture.params()["response-time"]["controller"].exists());
    controller.close();
    assert(fixture.params()["response-time"]["controller"].exists());
  }

  // (The original close() calls its callback every time it is called, and loops if the callback closes the controller: the two blocks below are the new contract)

  // close() calls the callback, once, however many times it is called
  {
    Fixture fixture;
    Bare controller(*fixture);
    int called = 0;

    Route::attach(controller, [&]() { ++called; });
    assert(called == 0);
    controller.close();
    controller.close();
    controller.redirect_to("/x");     // closes too
    controller.respond_with(HttpStatus::gone);
    assert(called == 1);
    assert(controller.is_closed());
  }

  // The callback may close the controller again (finalizers do): that does not loop
  {
    Fixture fixture;
    Bare controller(*fixture);
    int called = 0;

    Route::attach(controller, [&]() { ++called; controller.close(); });
    controller.close();
    assert(called == 1);
  }

  return 0;
}

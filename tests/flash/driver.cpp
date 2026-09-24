#include <crails/environment.hpp>
#include <crails/controller/flash.hpp>
#include "../test_support.hpp"

#undef NDEBUG
#include <cassert>

using namespace Crails;
using namespace Support;
using namespace std;

struct Bare : public Crails::FlashController
{
  Bare(Crails::Context& context) : Crails::FlashController(context)
  {
  }

  template<typename VALUE>
  void set_flash(const std::string& key, VALUE value)
  {
    flash[key] = value;
  }

  Data get_flash(const std::string& key)
  {
    return received_flash[key];
  }

  using Crails::ActionController::session;
  using Crails::ActionController::close;
};

typedef Crails::ActionRoute<Bare> Route;

int main()
{
  Crails::environment = Crails::Test;
  std::optional<Setup> setup;
  setup.emplace();

  // A controller can pass values to the next loaded controller (with the same session)
  {
    Fixture fixture;
    Bare controller(*fixture);
    ActionRoute<Bare>::attach(controller, std::bind(&Bare::finalize, &controller));

    controller.session["tata"] = 42;
    controller.set_flash<std::string>("toto", "tintin");
    controller.close();
  }
  // Said values can be retrieved through received_flash
  {
    Fixture fixture;
    Bare controller(*fixture);
    ActionRoute<Bare>::attach(controller, std::bind(&Bare::finalize, &controller));

    assert(controller.session["tata"].exists()); // session works
    assert(controller.get_flash("toto").exists());
    assert(controller.get_flash("toto").as<std::string>() == "tintin");
    controller.close();
  }
  // The values are destroyed after the next controller completed
  {
    Fixture fixture;
    Bare controller(*fixture);
    ActionRoute<Bare>::attach(controller, std::bind(&Bare::finalize, &controller));
    assert(controller.get_flash("toto").exists() == false);
    controller.close();
  }
  setup.reset(); // reset everything for potential further testing
  setup.emplace();

  Crails::Server::cleanup();
  return 0;
}


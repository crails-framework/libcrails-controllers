#include "flash.hpp"

using namespace std;
using namespace Crails;

FlashController::FlashController(Context& context) : Super(context)
{
  vars["flash"] = &received_flash;
  if (session["flash"].exists())
  {
    received_flash.as_data().merge(session["flash"]);
    session["flash"].destroy();
  }
}

void FlashController::finalize()
{
  session["flash"].merge(flash);
}

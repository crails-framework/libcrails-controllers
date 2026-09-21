#ifndef  CRAILS_CONTROLLER_HPP
# define CRAILS_CONTROLLER_HPP

# include "controller/action.hpp"
# include "controller/basic_authentication.hpp"
# include "controller/render.hpp"
# include "controller/flash.hpp"
# include "controller/csrf.hpp"
# include "controller/coroutine.hpp"

namespace Crails
{
  typedef FlashController     SynchronousController;
  typedef CoroutineController Controller;
}

#endif

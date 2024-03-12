#ifndef  CRAILS_FLASH_CONTROLLER_HPP
# define CRAILS_FLASH_CONTROLLER_HPP

# include "render.hpp"

namespace Crails
{
  class FlashController : public RenderController
  {
    typedef RenderController Super;
  public:
    FlashController(Context&);

    virtual void finalize() override;

  protected:
    DataTree received_flash;
    DataTree flash;
  };
}

#endif

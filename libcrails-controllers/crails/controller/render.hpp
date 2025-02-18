#ifndef  CRAILS_RENDER_CONTROLLER_HPP
# define CRAILS_RENDER_CONTROLLER_HPP

# include "action.hpp"
# include "csrf.hpp"
# include <crails/shared_vars.hpp>

namespace Crails
{
  class RenderController : public CsrfController
  {
  public:
    enum RenderType { TEXT, HTML, XML, JSON, JSONP, RAW };

    RenderController(Context&);

    void        render(const std::string& view);
    void        render(const std::string& view, SharedVars);
    void        render_accepting(const std::string& accept, const std::string& view, SharedVars);
    void        render(RenderType type, Data value);
    void        render(RenderType type, const std::string& value);
    std::string get_accept_header() const;

  private:
    void        set_content_type(RenderType);
  };
}

#endif

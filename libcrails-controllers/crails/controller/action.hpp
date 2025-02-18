#ifndef  CRAILS_ACTION_HANDLER_HPP
# define CRAILS_ACTION_HANDLER_HPP

# include <crails/utils/timer.hpp>
# include <crails/datatree.hpp>
# include <crails/http.hpp>
# include <crails/shared_vars.hpp>
# include <memory>
# include <functional>
# include <thread>

namespace Crails
{
  class Params;
  class Context;
  class BuildingResponse;
  template<typename CONTROLLER> class ActionRoute;

  class ActionController : public std::enable_shared_from_this<ActionController>
  {
    template<typename CONTROLLER>
    friend class ActionRoute;
  public:
    std::string get_controller_name() const;
    std::string get_action_name() const;
    const SharedVars& get_vars() const { return vars; }
  protected:
    ActionController(Context&);
    virtual ~ActionController();

    void redirect_to(Crails::HttpStatus status, const std::string& uri);
    void redirect_to(const std::string& uri) { redirect_to(HttpStatus::see_other, uri); }
    void respond_with(Crails::HttpStatus);
    virtual void initialize() {}
    virtual void finalize() {}
    void close();

    std::thread start_thread(std::function<void()> invokable);

    Params&            params;
    SharedVars&        vars;
    Data               session;
    const HttpRequest& request;
    BuildingResponse&  response;
  private:
    Utils::Timer             timer;
    std::shared_ptr<Context> context;
    std::function<void()>    callback;
    bool                     close_on_deletion = false;
  };
}

#endif

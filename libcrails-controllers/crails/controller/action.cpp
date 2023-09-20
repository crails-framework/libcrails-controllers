#include "action.hpp"
#include <crails/context.hpp>
#include <crails/logger.hpp>

using namespace std;
using namespace Crails;

ActionController::ActionController(Context& context) :
  params(context.params),
  session(params.get_session()),
  request(context.connection->get_request()),
  response(context.response),
  context(context.shared_from_this())
{
}

ActionController::~ActionController()
{
  logger << Logger::Debug << "ActionController::~ActionController: closing request on deletion: " << close_on_deletion << Logger::endl;
  if (close_on_deletion)
    close();
}

string ActionController::get_controller_name() const { return params["controller-data"]["name"].as<string>(); }
string ActionController::get_action_name() const     { return params["controller-data"]["action"].as<string>(); }

void ActionController::redirect_to(HttpStatus status, const string& uri)
{
  response.set_status_code(status);
  response.set_header(HttpHeader::location, uri);
  close();
}

void ActionController::respond_with(Crails::HttpStatus code)
{
  response.set_status_code(code);
  close();
}

void ActionController::close()
{
  if (callback)
  {
    logger << Logger::Debug << "ActionController::close()" << Logger::endl;
    params["response-time"]["controller"] = timer.GetElapsedSeconds();
    callback();
    callback = std::function<void()>();
  }
}

std::thread ActionController::start_thread(function<void()> invokable)
{
  shared_ptr<Context> context_ref = context;
  thread t([invokable, context_ref]()
  {
    context_ref->protect(invokable);
  });

  t.detach();
  return move(t);
}

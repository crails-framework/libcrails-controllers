#include "render.hpp"
#include <crails/shared_vars.hpp>
#include <crails/params.hpp>
#include <crails/renderer.hpp>
#include <crails/http_response.hpp>

using namespace std;
using namespace Crails;

const map<RenderController::RenderType, string> content_types{
  {RenderController::TEXT,  "text/plain"},
  {RenderController::HTML,  "text/html"},
  {RenderController::XML,   "text/xml"},
  {RenderController::JSON,  "application/json"},
  {RenderController::JSONP, "application/javascript"},
  {RenderController::RAW,   "application/octet-stream"}
};

RenderController::RenderController(Context& context) : CsrfController(context)
{
  // Set the class attributes accessible to the views
  vars["controller"] = this;
  vars["params"]     = &params;
  vars["session"]    = &session;
}

string RenderController::get_accept_header() const
{
  auto accept_header = request.find(HttpHeader::accept);

  string accept = accept_header != request.end()
    ? string(accept_header->value())
    : string();
  return accept_header != request.end()
    ? string(accept_header->value())
    : string();
}

void RenderController::render(const std::string& view)
{
  Renderer::render(view, get_accept_header(), response, vars);
  close();
}

void RenderController::render(const std::string& view, SharedVars vars)
{
  vars = merge(vars, this->vars);
  Renderer::render(view, get_accept_header(), response, vars);
  close();
}

void RenderController::render(RenderType type, const string& value)
{
  set_content_type(type);
  response.set_body(value.c_str(), value.length());
  close();
}

void RenderController::render(RenderType type, Data value)
{
  string       content_type;
  stringstream body;

  switch (type)
  {
    case TEXT:
      body << value.defaults_to<string>("");
      break ;
    case JSON:
      value.output(body);
      break ;
    case XML:
      body << value.to_xml();
      break ;
    default:
      throw boost_ext::invalid_argument("Crails::RenderController::render(RenderType,Data) only supports TEXT, XML and JSON");
  }
  set_content_type(type);
  response.set_body(body.str().c_str(), body.str().length());
  close();
}

void RenderController::set_content_type(RenderType type)
{
  response.set_header(HttpHeader::content_type, content_types.at(type));
}

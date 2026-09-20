#include "coroutine.hpp"
#include <crails/context.hpp>

using namespace Crails;

boost::asio::any_io_executor CoroutineController::get_io_executor()
{
  if (!context || !context->connection) [[unlikely]]
    throw boost_ext::runtime_error("Crails::Controller does not have a connection: can't get io_executor");
  return context->connection->get_strand();
}

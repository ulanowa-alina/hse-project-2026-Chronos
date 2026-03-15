#ifndef SERVER_SERVER_HPP
#define SERVER_SERVER_HPP

#include "db/connection_pool.hpp"
#include "session.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <string>

namespace asio = boost::asio;
namespace beast = boost::beast;
using tcp = asio::ip::tcp;

class Server {
  public:
    Server(asio::io_context& ioc, const std::string& host, unsigned short port,
           ConnectionPool& pool);

  private:
    void doAccept();

    tcp::acceptor acceptor_;
    Router router_;
    ConnectionPool& pool_;
};

#endif // SERVER_SERVER_HPP

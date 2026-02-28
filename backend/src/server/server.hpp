#ifndef SERVER_SERVER_HPP
#define SERVER_SERVER_HPP

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
using tcp = asio::ip::tcp;

class Server {
  public:
    Server(asio::io_context& ioc, unsigned short port);

  private:
    void doAccept();

    tcp::acceptor acceptor_;
};

#endif // SERVER_SERVER_HPP

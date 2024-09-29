#include "websocket.h"
#include "logging.h"

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast/websocket.hpp> // Correct WebSocket header
#include <boost/beast/core.hpp> // For flat_buffer and tcp_stream
#include <iomanip>
#include <iostream>

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;

int webSocketConnect(void)
{
    auto url = "ws.ifelse.io";
    // Always start with an I/O context object.
    boost::asio::io_context ioc{};

    boost::system::error_code ec{};
    tcp::resolver resolver{ ioc };
    auto resolverIt = resolver.resolve(url, "80", ec);
    if (ec) {
        Log(ec);
        return -1;
    }

    // Create a TCP socket
    tcp::socket socket{ ioc };
    socket.connect(*resolverIt, ec);

    if (ec) {
        Log(ec);
        return -1;
    }

    // Create the WebSocket stream using the socket
    websocket::stream<boost::beast::tcp_stream> tcpStream{ std::move(socket) };
    
    // Perform WebSocket handshake
    tcpStream.handshake(url, "/echo", ec);
    if (ec) {
        Log(ec);
        return -1;
    }

    tcpStream.text(true);

    auto msg = "Hello World";
    boost::asio::const_buffer wbuff{ msg, std::strlen(msg) };

    // Write the message
    tcpStream.write(wbuff, ec);
    if (ec) {
        Log(ec);
        return -1;
    }

    // Prepare buffer for the response
    boost::beast::flat_buffer rbuff{};
    
    // Read the echo response
    tcpStream.read(rbuff, ec);
    if (ec) {
        Log(ec);
        return -1;
    }

    std::cout << "ECHO: " << boost::beast::make_printable(rbuff.data()) << std::endl;

    return 0;
}

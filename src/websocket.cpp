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


int webSocketSyncConnect(void)
{
    auto url = "ws.ifelse.io";
    // Always start with an I/O context object.
    boost::asio::io_context ioc{};

    boost::system::error_code ec{};
    tcp::resolver resolver{ ioc };

    // Resolve the URL and port of the web socket server.
    auto resolverIt = resolver.resolve(url, "80", ec);
    if (ec) {
        Log(ec);
        return -1;
    }

    // Create a TCP socket and connect to resolved endpoint.
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

void OnHandshake(
    // --> Start of shared data
    websocket::stream<boost::beast::tcp_stream>& tcpStream,
    const boost::asio::const_buffer& wbuff,
    boost::beast::flat_buffer& rbuff,
    // <-- End of shared data
    const boost::system::error_code& ec
)
{
	std::cout << "inside handshake" << std::endl;
	boost::beast::error_code ec1;
    tcpStream.text(true);

    // Write the message
    tcpStream.write(wbuff, ec1);
    if (ec1) {
        Log(ec1);
    }
    
    // Read the echo response
    tcpStream.read(rbuff, ec1);
    if (ec1) {
        Log(ec1);
    }

    std::cout << "ECHO: " << boost::beast::make_printable(rbuff.data()) << std::endl;
}
void OnConnect(
    // --> Start of shared data
    websocket::stream<boost::beast::tcp_stream>& ws,
    const std::string& url,
    const std::string& endpoint,
    const boost::asio::const_buffer& wBuffer,
    boost::beast::flat_buffer& rBuffer,
    // <-- End of shared data
    const boost::system::error_code& ec
)
{
	std::cout << "inside async_connect" << std::endl;
    if (ec) {
        Log(ec);
        return;
    }

    // Attempt a WebSocket handshake.
    ws.async_handshake(url, endpoint,
        [&ws, &wBuffer, &rBuffer](auto ec) {
            OnHandshake(ws, wBuffer, rBuffer, ec);
        }
    );

}

int websocketAsyncConnect(void)
{
    auto url = "ws.ifelse.io";
    auto endpoint = "/";
    auto msg = "Hello World";	
    boost::asio::const_buffer wbuff{ msg, std::strlen(msg) }; // Prepare Write buffer
    boost::beast::flat_buffer rbuff{};	 // Prepare buffer for the response

    // Always start with an I/O context object.
    boost::asio::io_context ioc{};

    boost::system::error_code ec{};
    tcp::resolver resolver{ ioc };

    // Resolve the URL and port of the web socket server.
    auto resolverIt = resolver.resolve(url, "80", ec);
    if (ec) {
        Log(ec);
        return -1;
    }

#if 0
    // Create a TCP socket and connect to resolved endpoint.
    tcp::socket socket{ ioc };
    socket.connect(*resolverIt, ec);

    if (ec) {
        Log(ec);
        return -1;
    }
#endif
    // Create the WebSocket stream using the socket
    tcp::socket socket{ ioc };
    websocket::stream<boost::beast::tcp_stream> tcpStream{ std::move(socket) };
    
    // Connect to the TCP socket.
	// Instead of constructing the socket and the ws objects separately, the
	// socket is now embedded in ws, and we access it through next_layer().

	std::cout << "before async_connect" << std::endl;
	tcpStream.next_layer().async_connect(*resolverIt,
	    [&tcpStream, &url, &endpoint, &wbuff, &rbuff](auto ec) {
	        OnConnect(tcpStream, url, endpoint, wbuff, rbuff, ec);
	    }
	);

	ioc.run();
	#if 0
    // Perform WebSocket handshake
    tcpStream.handshake(url, "/echo", ec);
    if (ec) {
        Log(ec);
        return -1;
    }

    tcpStream.text(true);



    // Write the message
    tcpStream.write(wbuff, ec);
    if (ec) {
        Log(ec);
        return -1;
    }


    
    // Read the echo response
    tcpStream.read(rbuff, ec);
    if (ec) {
        Log(ec);
        return -1;
    }

    std::cout << "ECHO: " << boost::beast::make_printable(rbuff.data()) << std::endl;
#endif
    return 0;
}
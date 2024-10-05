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

namespace NetworkMonitor 
{
    WebSocketClient::WebSocketClient(
        const std::string& url,
        const std::string& endpoint,
        const std::string& port,
        boost::asio::io_context& ioc
    ) : url_ {url},
        endpoint_ {endpoint},
        port_ {port},
        resolver_ {boost::asio::make_strand(ioc)},
        ws_ {boost::asio::make_strand(ioc)}
    {
    }

    WebSocketClient:: ~WebSocketClient() = default;

    void WebSocketClient::connect(
        std::function<void (boost::system::error_code)> on_connect,
        std::function<void (boost::system::error_code,
                        std::string&&)> on_message,
        std::function<void (boost::system::error_code)> on_disconnect)
    {
        // Move the callbacks for later use
        OnDisconnect_   =std::move(on_connect);
        OnMessage_      =std::move(on_message);
        OnDisconnect_   =std::move(on_disconnect);

        // Start the chain of asynchronous callbacks
        Resolver.async_resolve(
          url_,port_,
          [this](auto err_code, auto endpoint)
        {
          OnResolve(err_code,endpoint); // private function
        });
  }    

  void WebSocketClient::OnResolve(const boost::system::error_code& ec,
                                tcp::resolver::results_type results)
  {
    if (ec) {
        if (onConnect_) {
            onConnect_(ec);
        }
        return;
    }

    // Connect to the TCP socket
    ws_.next_layer().async_connect(*results,
        [this](auto ec) {
            OnConnect(ec);
        });
  }

  void WebSocketClient::OnConnect(const boost::system::error_code& ec)
  {
    if (ec) {
        if (onConnect_) {
            onConnect_(ec);
        }
        return;
    }

    // Perform WebSocket handshake
    ws_.async_handshake(url_, endpoint_,
        [this](auto ec) {
            if (onConnect_) {
                onConnect_(ec);
            }

            // Start listening for messages
            ListenToIncomingMessage(ec);
        });
  }

  void WebSocketClient::ListenToIncomingMessage(
    const boost::system::error_code& ec
  )
  { 
    // Stop processing messages if the connection has been aborted.
    if (ec == boost::asio::error::operation_aborted) {
        // We check the closed_ flag to avoid notifying the user twice.
        if (onDisconnect_ && ! closed_) {
            onDisconnect_(ec);
        }
        return;
    }

    // Read a message asynchronously. On a successful read, process the message
    // and recursively call this function again to process the next message.
    ws_.async_read(rBuffer_,
        [this](auto ec, auto nBytes) {
            OnRead(ec, nBytes);
            ListenToIncomingMessage(ec);
        }
    );
  }

  void WebSocketClient::OnRead(
    const boost::system::error_code& ec,
    size_t nBytes
  )
  {
    // We just ignore messages that failed to read.
    if (ec) {
        return;
    }

    // Parse the message and forward it to the user callback.
    // Note: This call is synchronous and will block the WebSocket strand.
    std::string message {boost::beast::buffers_to_string(rBuffer_.data())};
    rBuffer_.consume(nBytes);
    if (onMessage_) {
        onMessage_(ec, std::move(message));
    }
  }


}

int webSocketSyncConnect(void)
{
    auto url = "ws.ifelse.io";
    
    // Step 1. Always start with an I/O context object.
    boost::asio::io_context ioc{};

    boost::system::error_code ec{};
    tcp::resolver resolver{ ioc };

    // Step 2. Resolve the URL and port of the web socket server.
    auto resolverIt = resolver.resolve(url, "80", ec);
    if (ec) {
        Log(ec);
        return -1;
    }

    // Step 3. Create a TCP socket and connect to resolved endpoint.
    tcp::socket socket{ ioc };
    socket.connect(*resolverIt, ec);

    if (ec) {
        Log(ec);
        return -1;
    }

    // Create the WebSocket stream using the socket
    websocket::stream<boost::beast::tcp_stream> tcpStream{ std::move(socket) };
    
    // Step 4. Perform WebSocket handshake
    tcpStream.handshake(url, "/echo", ec);
    if (ec) {
        Log(ec);
        return -1;
    }

    tcpStream.text(true);

    auto msg = "Hello World";
    boost::asio::const_buffer wbuff{ msg, std::strlen(msg) };

    // Step 5. Now, Write the message
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

    // Step 1. Always start with an I/O context object.
    boost::asio::io_context ioc{};

    boost::system::error_code ec{};

    // Step 2. Create the resolver
    tcp::resolver resolver{ ioc };

    // Step 2.1. Resolve the URL and port of the web socket server.
    auto resolverIt = resolver.resolve(url, "80", ec);
    if (ec) {
        Log(ec);
        return -1;
    }

    // Step 3. Create the WebSocket stream using the socket (here we delegate the socket managment websocket::stream)
    tcp::socket socket{ ioc };
    websocket::stream<boost::beast::tcp_stream> tcpStream{ std::move(socket) };
    
    // Step 3. Connect to the TCP socket.
	// Instead of constructing the socket and the ws objects separately, the
	// socket is now embedded in ws, and we access it through next_layer().
    // Also the thing to note here is we using lambda here since second param of async_connect takes 
    // a function ptr with one param but we need to pass many to the callback so we catch that 
    // through environment and pass into lambda. So that on async_connect is successful, it would call
    // the lambda passed which is eventually calling OnConnect.
	std::cout << "before async_connect" << std::endl;
	tcpStream.next_layer().async_connect(*resolverIt,
	    [&tcpStream, &url, &endpoint, &wbuff, &rbuff](auto ec) {
	        OnConnect(tcpStream, url, endpoint, wbuff, rbuff, ec);
	    }
	);

	ioc.run();
    return 0;
}
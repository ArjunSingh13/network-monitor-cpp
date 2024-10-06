#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast/websocket.hpp> // Correct WebSocket header
#include <boost/beast/core.hpp> // For flat_buffer and tcp_stream
#include <string>
#include "websocket-client.h"


using tcp = boost::asio::ip::tcp;
//namespace websocket = boost::beast::websocket;

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

    void WebSocketClient::Connect(
        std::function<void (boost::system::error_code)> on_connect,
        std::function<void (boost::system::error_code,
                        std::string&&)> on_message,
        std::function<void (boost::system::error_code)> on_disconnect)
    {
        // Move the callbacks for later use
        OnConnect_   =std::move(on_connect);
        OnMessage_      =std::move(on_message);
        OnDisconnect_     =std::move(on_disconnect);

        // Step 1. Start the chain of asynchronous callbacks
        resolver_.async_resolve(
          url_,port_,
          [this](auto err_code, auto endpoint)
        {
          std::cout << "About to call OnResolve" << std::endl;
          OnResolve(err_code,endpoint); // private function
        });
  }    

  void WebSocketClient::OnResolve(const boost::system::error_code& ec,
                                typename tcp::resolver::results_type results)
  {
    std::cout << "Entered OnResolve" << std::endl;
    if (ec) {
        std::cout << "Error in OnResolve " << ec << std::endl;
        if (OnConnect_) {
            OnConnect_(ec);
        }
        return;
    }

          std::cout << "About to call async_connect" << std::endl;
    // Step 2. Connect to the TCP socket
    ws_.next_layer().async_connect(*results,
        [this](auto ec) {
            OnConnect(ec);
        });
  }

  void WebSocketClient::OnConnect(const boost::system::error_code& ec)
  {
    if (ec) {
        if (OnConnect_) {
            OnConnect_(ec);
        }
        return;
    }

    std::cout << "About to do handshake" << std::endl;
    // Step 3. Perform WebSocket handshake
    ws_.async_handshake(url_, endpoint_,
        [this](auto ec) {
            if (OnConnect_) {
                OnConnect_(ec);  // Here we calling OnConnect to send message.
            }

            // Start listening for messages
            ListenToIncomingMessage(ec); // Here we listening recursively for incoming messages.
        });
  }

  void WebSocketClient::ListenToIncomingMessage(
    const boost::system::error_code& ec
  )
  { 
    // Stop processing messages if the connection has been aborted.
    if (ec == boost::asio::error::operation_aborted) {
        // We check the closed_ flag to avoid notifying the user twice.
        if (OnDisconnect_   && ! closed_) {
            OnDisconnect_ (ec);
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
    if (OnMessage_) {
        OnMessage_(ec, std::move(message));
    }
  }

  void WebSocketClient::Send(const std::string& message,
                           std::function<void(boost::system::error_code)> onSend) {
    ws_.async_write(boost::asio::buffer(message),
        [onSend](auto ec, auto /*bytes_transferred*/) {
            // Dispatch the user callback synchronously, blocking the strand
            if (onSend) {
                onSend(ec);  // User callback for handling message sent status
            }
        }
    );
  }

  void WebSocketClient::Close(
    std::function<void(boost::system::error_code)> onClose) {
    
    // Set the closed_ flag to true to indicate we're closing the connection
    closed_ = true;

    // Store the onClose callback for later use
  //  if (onClose) {
  //      OnDisconnect_ = std::move(onClose);
  //  }

    // Initiate the WebSocket close handshake
    ws_.async_close(boost::beast::websocket::close_code::normal,
        [this](boost::system::error_code ec) {
            // Handle the result of the close operation
        // Call the user-provided callback to notify that the WebSocket has been closed
        if (OnDisconnect_) {
            OnDisconnect_(ec);  // Pass the error code to the user's callback
        }
        });
}

void NetworkMonitor::WebSocketClient::OnClose(const boost::system::error_code& ec) {
    if (ec) {
        std::cerr << "Error closing WebSocket: " << ec.message() << std::endl;
    } else {
        std::cout << "WebSocket closed successfully." << std::endl;
    }

    // Call the user-provided callback to notify that the WebSocket has been closed
    if (OnDisconnect_) {
        OnDisconnect_(ec);  // Pass the error code to the user's callback
    }
}


}

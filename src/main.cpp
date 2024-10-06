#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <iomanip>
#include <iostream>
#include <thread>

#include "websocket-client.h"
#include "logging.h"

using tcp = boost::asio::ip::tcp;


int main()
{
  std::cerr << "[" << std::setw(14) << std::this_thread::get_id() << "] main"
    << std::endl;

  const std::string url {"ws.ifelse.io"};
  const std::string endpoint {"/"};
  const std::string port {"80"};
  boost::asio::io_context ioc {};
  const std::string message {"Hello World Kida"};
  
  bool connected {false};
  bool messageSent {false};
  bool messageReceived {false};
  bool disconnected {false};
  NetworkMonitor::WebSocketClient webClient{url, endpoint, port, ioc};

  auto onDisconnect = [&disconnected](boost::system::error_code ec) {
    if (!ec) {
        // Connection closed successfully
        std::cout << "Disconnected from the WebSocket server successfully." << std::endl;
        disconnected = true;
    } else {
        // An error occurred during disconnection
        std::cout << "Error during disconnection: " << ec.message() << std::endl;
    }
  };

  auto onConnect = [&webClient, &connected, &message, &messageSent](boost::system::error_code ec){

        static int count = 0;
        std::cout << "onConnect in main.cpp is called " << count++ << std::endl;
        if(!ec)
        {
          std::cout << "Connected Successfully" << std::endl;
          connected = true;

          webClient.Send(message, 
                         [&messageSent](boost::system::error_code ec)
                         {
                            if(!ec) 
                            {
                               std::cout << "Message Sent Successfully" << std::endl;
                               messageSent = true;
                            }

                         }
                         );
        }
        else
        {
            std::cout << "OnConnect: Connection failed" << std::endl; 
        }
  };

  auto onReceive = [&webClient, &messageReceived, &disconnected] (boost::system::error_code ec, std::string&& received){
        if(!ec){
            std::cout << "Message received" << received << std::endl;
            messageReceived = true;

            // Close the connection after receiving a message
            webClient.Close([&disconnected](boost::system::error_code ec) {
                if (!ec) {
                    std::cout << "Connection closed successfully." << std::endl;
                    disconnected = true;
                } else {
                    std::cout << "Error closing connection: " << ec.message() << std::endl;
                }
            });
        }
        else
        {
            std::cout << "onReceive connection failed" << std::endl;
        }

  };

  std::cout << "About to call webClient.Connect" << std::endl;
  webClient.Connect(onConnect, onReceive, onDisconnect);
  ioc.run();


  return 0;
}
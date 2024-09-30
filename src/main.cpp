#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <iomanip>
#include <iostream>
#include <thread>

#include "websocket.h"
#include "logging.h"

using tcp = boost::asio::ip::tcp;

void OnConnect(boost::system::error_code ec)
{
    Log(ec);
}

int main()
{
    std::cerr << "[" << std::setw(14) << std::this_thread::get_id() << "] main"
        << std::endl;

websocketAsyncConnect();
#if 0 // Practicing multiple threads for dividing work.
    // Always start with an I/O context object.
    boost::asio::io_context ioc{};

    // Create an I/O object. Every Boost.Asio I/O object API needs an io_context
    // as the first parameter.
    tcp::socket socket{ boost::asio::make_strand(ioc) };

    size_t nThreads{ 4 };

    // Under the hood, socket.connect uses I/O context to talk to the socket
    // and get a response back. The response is saved in ec.
    boost::system::error_code ec{};
    tcp::resolver resolver{ ioc };
    auto resolverIt{ resolver.resolve("google.com", "80", ec) };
    if (ec) {
        Log(ec);
        return -1;
    }
    for (size_t idx{ 0 }; idx < nThreads; ++idx) {
        socket.async_connect(*resolverIt, OnConnect);
    }

    // We must call io_context::run for asynchronous callbacks to run.
    // Below [&ioc]() {} is a lambda function capturing ioc by reference from environment and passing to thread.
    // Note: Why passing lambda function to thread? So thread here needed a function to be passed. since what we
    // want to achieve from this thread is very limited (just ioc.run() ) its more convenient to use lambda here.
    // And you cant pass ioc.run() directly since this function is part of a different class and first we need
    // to know the object before running a function of the class that that object belongs to.
    std::vector<std::thread> threads{};
    threads.reserve(nThreads); // reserving fixed memory in advance so that we dont need to use dynamic memory allocation.
    for (size_t idx{ 0 }; idx < nThreads; ++idx) {
        threads.emplace_back([&ioc]() {
            ioc.run();
            });
    }

    for (size_t idx{ 0 }; idx < nThreads; ++idx) {
        threads[idx].join();
    }
#endif
    return 0;
}
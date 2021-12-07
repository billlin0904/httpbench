#include "server.h"
#include "client.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace bench {

std::shared_ptr<HttpServer> make_http_server(net::io_context &ioc, const char *bind_port) {
    auto const address = net::ip::make_address("127.0.0.1");
    auto const port = static_cast<unsigned short>(std::atoi(bind_port));
    auto const doc_root = std::make_shared<std::string>("/html");

    auto server = std::make_shared<bench::HttpServer>(
        ioc,
        tcp::endpoint{ address, port },
        doc_root);    
    server->run();

    std::cout << "Http server lisen:" << address << ":" << port << std::endl;
    std::cout << "Press Ctrl+C to stop server!" << std::endl;   

    return server;
}

std::shared_ptr<HttpClient> make_http_client(net::io_context& ioc, const char* bind_port) {
    auto client = std::make_shared<bench::HttpClient>(ioc);
    client->run("127.0.0.1", bind_port, "/", 11);
    return client;
}

}

int main() {
    auto const threads = std::thread::hardware_concurrency();
    const char* port = "5050";
    size_t client_count = 100;

    net::io_context server_ioc(threads);
    net::signal_set signals(server_ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&](beast::error_code const&, int) {
            server_ioc.stop();
            std::cout << "Http server was stopped." << std::endl;
        });

    auto server = bench::make_http_server(server_ioc, port);
    std::vector<std::thread> server_threads;
    server_threads.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
        server_threads.emplace_back([&server_ioc] {
        server_ioc.run();
            });

    /*net::io_context client_ioc(threads);
    std::vector<std::shared_ptr<bench::HttpClient>> clients;
    clients.reserve(client_count);
    for (size_t i = 0; i < client_count; ++i) {
        clients.push_back(bench::make_http_client(server_ioc, port));
    }

    std::vector<std::thread> client_threads;
    client_threads.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
        client_threads.emplace_back([&client_ioc] {
        client_ioc.run();
            });*/

    server_ioc.run();
    //client_ioc.run();

    for (auto& t : server_threads) {
        t.join();
    }    

    /*for (auto& t : client_threads) {
        t.join();
    }*/
}
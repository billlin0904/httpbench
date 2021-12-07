#include "server.h"
#include "client.h"
#include "httpstatis.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace bench {

std::shared_ptr<HttpServer> make_http_server(net::io_context &ioc, const char* host, const char *bind_port) {
    auto const address = net::ip::make_address(host);
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

std::shared_ptr<HttpClient> make_http_client(net::io_context& ioc, const char* host, const char* bind_port) {
    auto client = std::make_shared<bench::HttpClient>(ioc);
    client->run(host, bind_port, "/", 11);
    return client;
}

}

int main() {
    auto const threads = std::thread::hardware_concurrency();
    const char* port = "5050";
    const char* host = "127.0.0.1";
    size_t client_count = 100;
    size_t num_test_request = 5000000;

    bench::HttpStatis::get().set_test_request_size(
        num_test_request,
        client_count,
        threads);
    
    net::io_context client_ioc(threads);
    net::io_context server_ioc(threads);

    net::signal_set signals(server_ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&](beast::error_code const&, int) {
            server_ioc.stop();
            client_ioc.stop();
            std::cout << "Http server was stopped." << std::endl;
        });

    auto server = bench::make_http_server(server_ioc, host, port);
    std::vector<std::thread> server_threads;
    server_threads.reserve(threads);
    for (auto i = 0; i < threads; ++i) {
        server_threads.emplace_back([&server_ioc] {
            server_ioc.run();
            });
    }
    
    std::vector<std::shared_ptr<bench::HttpClient>> clients;
    clients.reserve(client_count);
    for (size_t i = 0; i < client_count; ++i) {
        clients.push_back(bench::make_http_client(server_ioc, host, port));
    }

    std::vector<std::thread> client_threads;
    client_threads.reserve(threads);
    for (auto i = 0; i < threads; ++i) {
        client_threads.emplace_back([&client_ioc] {
            client_ioc.run();
            });
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (bench::HttpStatis::get().stop_test()) {
            bench::HttpStatis::get().show_statistic();
            server_ioc.stop();
            client_ioc.stop();
            break;
        }
    }

    for (auto& t : server_threads) {
        t.join();
    }    

    for (auto& t : client_threads) {
        t.join();
    }
}
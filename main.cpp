#include "server.h"
#include "client.h"
#include "httpstatis.h"

#include <boost/program_options.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace program_options = boost::program_options;

namespace bench {

std::shared_ptr<HttpServer> make_http_server(net::io_context &ioc, const std::string &host, const std::string& bind_port) {
    auto const address = net::ip::make_address(host);
    auto const port = static_cast<unsigned short>(std::atoi(bind_port.c_str()));
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

std::shared_ptr<HttpClient> make_http_client(net::io_context& ioc, const std::string& host, const std::string& bind_port, const std::string& request_path) {
    auto client = std::make_shared<bench::HttpClient>(ioc);
    client->run(host.c_str(), bind_port.c_str(), request_path.c_str(), 11);
    return client;
}

}

int main(int argc, char *argv[]) {
    auto threads = std::thread::hardware_concurrency();
    std::string port = "5050";
    std::string host = "127.0.0.1";
    size_t client_count = 100;
    size_t num_test_request = 500000;
    std::string request_path = "/version";

    program_options::options_description options("Test Options");
    options.add_options()
        ("help", "httpbench --v both --s 127.0.0.1 --p 5050 --t 8 --n 500000 --c 100")        
        ("v", program_options::value<std::string>(), "'server' or 'client' or 'both'")
        ("s", program_options::value<std::string>(), "host")
        ("p", program_options::value<std::string>(), "port")
        ("t", program_options::value<size_t>(), "number of thread")
        ("n", program_options::value<size_t>(), "number of test request")
        ("c", program_options::value<size_t>(), "number of concurrent client");

    program_options::variables_map options_var;

    try {
        program_options::store(program_options::parse_command_line(argc, argv, options), options_var);
    }
    catch (std::exception const& e) {
        std::cout << e.what() << std::endl;
        return -1;
    }

    program_options::notify(options_var);
    bool is_server = false;
    bool is_client = false;

    if (options_var.count("help")) {
        std::cout << options << std::endl;
        return 1;
    }
    if (options_var.count("v")) {
        auto type = options_var["v"].as<std::string>();
        if (type == "server") {
            is_server = true;
        } else if (type == "client") {
            is_client = true;
        } else if (type == "both") {
            is_server = true;
            is_client = true;
        }
    }

    if (options_var.count("s")) {
        host = options_var["s"].as<std::string>();
    }
    if (options_var.count("p")) {
        port = options_var["p"].as<std::string>();
    }
    if (options_var.count("t")) {
        threads = options_var["t"].as<size_t>();
    }
    if (options_var.count("n")) {
        num_test_request = options_var["n"].as<size_t>();
    }
    if (options_var.count("c")) {
        client_count = options_var["c"].as<size_t>();
    }

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

    std::vector<std::thread> server_threads;
    std::shared_ptr<bench::HttpServer> server;
    if (is_server) {
        server = bench::make_http_server(server_ioc, host, port);

        server_threads.reserve(threads);
        for (auto i = 0; i < threads; ++i) {
            server_threads.emplace_back([&server_ioc] {
                server_ioc.run();
                });
        }
    }    
       
    std::vector<std::shared_ptr<bench::HttpClient>> clients;
    std::vector<std::thread> client_threads;

    if (is_client) {
        clients.reserve(client_count);
        for (size_t i = 0; i < client_count; ++i) {
            clients.push_back(bench::make_http_client(client_ioc, host, port, request_path));
        }
        client_threads.reserve(threads);
        for (auto i = 0; i < threads; ++i) {
            client_threads.emplace_back([&client_ioc] {
                client_ioc.run();
                });
        }
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
        if (!t.joinable()) {
            continue;
        }
        t.join();
    }    

    for (auto& t : client_threads) {
        if (!t.joinable()) {
            continue;
        }
        t.join();
    }
}
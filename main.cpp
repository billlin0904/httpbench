#include "server.h"

int main() {	
    auto const threads = std::thread::hardware_concurrency();
    auto const address = net::ip::make_address("0.0.0.0");
    auto const port = static_cast<unsigned short>(std::atoi("5050"));
    auto const doc_root = std::make_shared<std::string>("/html");

    net::io_context ioc(threads);

    std::make_shared<TcpListener>(
        ioc,
        tcp::endpoint{ address, port },
        doc_root)->run();    

    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&](beast::error_code const&, int) {
            ioc.stop();
            std::cout << "Http server was stopped." << std::endl;
        });

    std::cout << "Http server lisen:" << address << ":" << port << std::endl;
    std::cout << "Press Ctrl+C to stop server!" << std::endl;

    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
        v.emplace_back([&ioc] {
                ioc.run();
            });
    ioc.run();

    for (auto& t : v) {
        t.join();
    }
}
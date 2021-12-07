#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>

namespace bench {

namespace beast = boost::beast;         // from <boost/beast.hpp>

inline void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

inline void system_error(int ec) {
    std::cerr << std::system_category().message(ec) << "\n";
}

}
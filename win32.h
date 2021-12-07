#pragma once

#include <Windows.h>
#include <mstcpip.h>
#include <WinSock2.h>

#include "error.h"

namespace bench {

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>

static void enable_fast_loopback(SOCKET socket) {
    int option_value = 1;
    DWORD number_of_bytes_returned = 0;
    auto status = WSAIoctl(socket,
        SIO_LOOPBACK_FAST_PATH,
        &option_value,
        sizeof(option_value),
        nullptr,
        0,
        &number_of_bytes_returned,
        0,
        0);
    if (SOCKET_ERROR == status) {
        system_error(::GetLastError());
    }
}

static void excluse_address(tcp::acceptor & acceptor, boost::system::error_code& ec) {
    typedef boost::asio::detail::socket_option::boolean<BOOST_ASIO_OS_DEF(SOL_SOCKET), SO_EXCLUSIVEADDRUSE> excluse_address;
    acceptor.set_option(excluse_address(true), ec);
}

}
module;

#include <boost/asio.hpp>
#include <coroutine>
#include <ranges>
#include <span>

export module send_packet;

import packet;

using boost::asio::ip::udp;

export struct SendAwaitable {
  udp::socket &socket;
  Packet packet;
  std::vector<std::pair<udp::endpoint, std::string>> clients;

  struct Awaiter {
    udp::socket &socket;
    Packet &packet;
    std::vector<std::pair<udp::endpoint, std::string>> clients;

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> h) {
      auto span = packet.get_span();
      for (const auto &client : clients) {
        socket.async_send_to(
            boost::asio::buffer(span.data(), span.size_bytes()), client.first,
            [h](const boost::system::error_code &, std::size_t) mutable {
              h();
            });
      }
    }

    void await_resume() const noexcept {}
  };

  auto operator co_await() const {
    return Awaiter{socket, const_cast<Packet &>(packet), clients};
  }
};

export auto send_packet_to_clients(udp::socket &socket,
                                   std::span<std::byte> data,
                                   std::ranges::forward_range auto &&clients)
    -> SendAwaitable {
  return SendAwaitable{socket, Packet(data),
                       std::forward<decltype(clients)>(clients)};
}

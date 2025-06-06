module;

#include <boost/asio.hpp>
#include <boost/cobalt/promise.hpp>
#include <ranges>
#include <span>

export module send_packet;

import packet;

using boost::asio::ip::udp;

// export struct SendAwaitable {
//   udp::socket &socket;
//   Packet packet;
//   std::vector<std::pair<udp::endpoint, std::string>> clients;

//   struct Awaiter {
//     udp::socket &socket;
//     Packet &packet;
//     std::vector<std::pair<udp::endpoint, std::string>> clients;

//     boost::cobalt::promise<void> operator co_await() {
//       auto span = packet.get_span();
//       std::vector<boost::cobalt::promise<void>> promises;

//     }
//   };

//   auto operator co_await() const {
//     return Awaiter{socket, const_cast<Packet &>(packet), clients};
//   }
// };

// export auto send_packet_to_clients(udp::socket &socket,
//                                    std::span<std::byte> data,
//                                    std::ranges::forward_range auto &&clients)
//     -> promise<void>{
//   Packet packet{data};
//   auto span = packet.get_span();

//   std::vector<promise<void>> promises;

//   for (const auto& client : clients) {
//     promise<void> p;
//     socket.async_send_to(
//       boost::asio::buffer(span.data(), span.size_bytes()), client.first,
//       [pr = p.get_resolver]
//     )
//   }
// }

export boost::cobalt::promise<void>
send_packet_to_clients(udp::socket &socket, std::span<std::byte> data,
                       std::ranges::forward_range auto &&clients) {
  Packet packet(data);
  auto span = packet.get_span();

  for (const auto &client : clients) {
    co_await socket.async_send_to(
        boost::asio::buffer(span.data(), span.size_bytes()), client.first,
        boost::asio::use_awaitable);
  }
}

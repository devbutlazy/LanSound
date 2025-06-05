module;

#include <cstddef>
#include <span>
#include <vector>

export module packet;

export struct Packet {
  std::vector<std::byte> data;

  Packet(std::span<const std::byte> input) {
    data = std::vector<std::byte>(input.begin(), input.end());
  }

  std::span<const std::byte> get_span() const noexcept { return data; }
};

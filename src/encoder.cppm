module;
#include "opus/opus.h"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <string_view>

export module encoder;

using std::size_t, std::uint8_t, std::uint16_t, std::string_view, std::byte,
    std::unique_ptr;

using std::string_view_literals::operator""sv;

template <typename O, typename T, typename U>
  requires std::convertible_to<T, bool> && std::convertible_to<T, O> &&
           std::convertible_to<U, O>
constexpr O some_or(T &&val, U &&otherwise) {
  if (static_cast<bool>(val)) {
    return static_cast<O>(val);
  }
  return static_cast<O>(otherwise);
}

struct Empty {};
export template <typename T = Empty>
  requires std::destructible<T> && std::move_constructible<T>
struct EncoderError {
  const string_view message;
  const int error;
  static constexpr int CUSTOM_ERROR = -1;
  // is zero-sized most of the time
  [[no_unique_address]]
  T owned_data;

  explicit EncoderError(const int err)
      : message(some_or<string_view>(opus_strerror(err), "Unknown error"sv)),
        error(err) {}

  explicit constexpr EncoderError(const string_view msg)
      : message(msg), error(CUSTOM_ERROR) {}

  EncoderError(const string_view msg, T &&data)
      : message(msg), error(CUSTOM_ERROR), owned_data(std::move(data)) {}

  // might be incorrect :)
  template <std::enable_if<std::is_copy_constructible_v<T>>>
  EncoderError(const EncoderError &) = delete;
};

std::expected<int, EncoderError<>> check_error(int code) {
  if (code != OPUS_OK) {
    return std::unexpected(EncoderError(code));
  }

  return code;
}

export enum class EncoderChannels : uint8_t {
  Mono = 1,
  Stereo = 2,
};

export class Encoder {
private:
  const uint8_t channels;
  unique_ptr<uint16_t[]> pcm_buffer_;
  unique_ptr<byte[]> opus_encoder_state_owned_;
  OpusEncoder *const opus_encoder_state_;

  explicit Encoder(const uint8_t channels)
      : channels(channels),
        pcm_buffer_(std::make_unique_for_overwrite<uint16_t[]>(
            FRAME_SAMPLES_PER_CHANNEL * channels)),
        opus_encoder_state_owned_(std::make_unique_for_overwrite<byte[]>(
            opus_encoder_get_size(channels))),
        // OpusEncoder's expected alignment is 1 and it's unsized.
        opus_encoder_state_(
            reinterpret_cast<OpusEncoder *>(opus_encoder_state_owned_.get())) {}

public:
  static constexpr size_t FRAME_DURATION_MS = 10;
  static constexpr size_t SAMPLE_RATE = 48000;
  static constexpr size_t FRAME_SAMPLES_PER_CHANNEL =
      FRAME_DURATION_MS * (SAMPLE_RATE / 1000);

  static constexpr size_t PACKET_LIMIT = 480;

  Encoder(Encoder &&) = default;

  static std::expected<Encoder, EncoderError<>>
  make(const EncoderChannels channels) {
    Encoder ins = Encoder(static_cast<uint8_t>(channels));

    auto res =
        check_error(opus_encoder_init(ins.opus_encoder_state_, SAMPLE_RATE,
                                      ins.channels, OPUS_APPLICATION_AUDIO))
            .and_then([&ins](int) {
              // only limited by output buffer size
              return check_error(opus_encoder_ctl(
                  ins.opus_encoder_state_, OPUS_SET_BITRATE(OPUS_BITRATE_MAX)));
            })
            .and_then([&ins](int) {
              return check_error(opus_encoder_ctl(ins.opus_encoder_state_,
                                                  OPUS_SET_COMPLEXITY(10)));
            })
            .and_then([&ins](int) {
              return check_error(opus_encoder_ctl(
                  ins.opus_encoder_state_, OPUS_SET_PACKET_LOSS_PERC(25)));
            })
            .and_then([&ins](int) {
              return check_error(
                  opus_encoder_ctl(ins.opus_encoder_state_, OPUS_SET_DTX(1)));
            });

    if (!res.has_value())
      return std::unexpected(res.error());

    return std::move(ins);
  }

  // todo: encode
  // todo: encode that pushes into a queue and launches sender
};
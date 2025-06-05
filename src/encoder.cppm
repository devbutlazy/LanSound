module;

#include "opus/opus.h"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <string_view>

export module encoder;

using std::size_t, std::uint8_t, std::uint16_t, std::string_view, std::byte,
    std::unique_ptr;

using std::string_view_literals::operator""sv;

struct Empty {};
export template <typename T = Empty>
  requires std::is_destructible_v<T> && std::is_move_constructible_v<T>
struct EncoderError {
  const int error;
  const string_view message;
  // is zero-sized most of the time
  [[no_unique_address]]
  T owned_data;

  explicit EncoderError(const int err)
      : error(err),
        message(opus_strerror(err) == nullptr ? "Unknown error"sv
                                              : opus_strerror(err)) {}

  explicit constexpr EncoderError(const string_view msg)
      : error(0), message(msg) {}

  EncoderError(const string_view msg, T &&data)
      : error(0), message(msg), owned_data(std::move(data)) {}

  // might be incorrect :)
  template <std::enable_if<std::is_copy_constructible_v<T>>>
  EncoderError(const EncoderError &) = delete;
};

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

  static auto make(const EncoderChannels channels)
      -> std::expected<Encoder, EncoderError<>> {
    Encoder ins = Encoder(static_cast<uint8_t>(channels));

#define CHECK_ERROR(var)                                                       \
  if (var != OPUS_OK) {                                                        \
    return std::unexpected(EncoderError<>(var));                               \
  }

    int ret = opus_encoder_init(ins.opus_encoder_state_, SAMPLE_RATE,
                                ins.channels, OPUS_APPLICATION_AUDIO);
    CHECK_ERROR(ret);

    // only limited by output buffer size
    ret = opus_encoder_ctl(ins.opus_encoder_state_,
                           OPUS_SET_BITRATE(OPUS_BITRATE_MAX));
    CHECK_ERROR(ret);
    ret = opus_encoder_ctl(ins.opus_encoder_state_, OPUS_SET_COMPLEXITY(10));
    CHECK_ERROR(ret);
    ret = opus_encoder_ctl(ins.opus_encoder_state_,
                           OPUS_SET_PACKET_LOSS_PERC(25));
    CHECK_ERROR(ret);
    ret = opus_encoder_ctl(ins.opus_encoder_state_, OPUS_SET_DTX(1));
    CHECK_ERROR(ret);

    return std::move(ins);
  }
};

#include <array>
#include <atomic>
#include <iostream>
#include <string>
#include <opus/opus.h>
#include <vector>

const int SAMPLE_RATE = 48000;
const int FRAME_SIZE = 960; // ~20ms in 48kHz

template <typename T, size_t N>
class CircularQueue
{
private:
    std::array<T, N> buffer_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;

public:
    static_assert((N & (N - 1)) == 0, "N must be a power of 2");

    CircularQueue() : head_(0), tail_(0) {}

    bool push(const T &item)
    {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next_head = (head + 1) & (N - 1);

        if (next_head == tail_.load(std::memory_order_acquire))
        {
            return false; // full
        }

        buffer_[head] = item;
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    std::optional<T> pop()
    {
        size_t tail = tail_.load(std::memory_order_relaxed);

        if (tail == head_.load(std::memory_order_acquire))
        {
            return std::nullopt; 
        }

        T item = buffer_[tail];
        tail_.store((tail + 1) & (N - 1), std::memory_order_release);
        return item;
    }

    bool empty() const
    {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    bool full() const
    {
        return ((head_.load(std::memory_order_acquire) + 1) & (N - 1)) == tail_.load(std::memory_order_acquire);
    }
};

int main()
{
    char header[44];
    if (!std::cin.read(header, 44))
    {
        std::cerr << "Failed to read WAV header" << std::endl;
        return 1;
    }

    int channels = static_cast<unsigned char>(header[22]) | static_cast<unsigned char>(header[23]) << 8;
    if (channels != 1 && channels != 2)
    {
        std::cerr << "Unsupported number of channels: " << channels << std::endl;
        return 1;
    }

    std::cout << "Detected channels: " << channels << std::endl;

    int err;

    OpusEncoder *encoder = opus_encoder_create(SAMPLE_RATE, channels, OPUS_APPLICATION_AUDIO, &err);
    if (err != OPUS_OK)
    {
        std::cerr << "Opus encoder initialization failed" << opus_strerror(err) << std::endl;
        return 1;
    }

    const size_t pcm_bytes = FRAME_SIZE * channels * 2;

    std::vector<std::vector<unsigned char>> opus_packets;
    std::vector<unsigned char> input_pcm(pcm_bytes);
    std::vector<opus_int16> pcm(FRAME_SIZE * channels);
    std::vector<unsigned char> opus_data(4000); // Opus packet <= 4000 bytes

    while (std::cin.read(reinterpret_cast<char *>(input_pcm.data()), pcm_bytes))
    {
        for (size_t i = 0; i < pcm.size(); ++i)
        {
            pcm[i] = input_pcm[i * 2] | (input_pcm[i * 2 + 1] << 8);
        }

        int encoded_bytes = opus_encode(encoder, pcm.data(), FRAME_SIZE, opus_data.data(), opus_data.size());
        if (encoded_bytes < 0)
        {
            std::cerr << "Encoding failed: " << opus_strerror(encoded_bytes) << std::endl;
            break;
        }

        opus_packets.emplace_back(opus_data.begin(), opus_data.begin() + encoded_bytes);
    }

    opus_encoder_destroy(encoder);

    std::cout << "Encoded " << opus_packets.size() << " packets.\n";
    return 0;
}

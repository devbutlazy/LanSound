#include <array>
#include <atomic>
#include <iostream>
#include <memory>
#include <optional>
#include <opus/opus.h>
#include <span>
#include <string>
#include <vector>

const int SAMPLE_RATE = 48000;
const int FRAME_SIZE = 960; // ~20ms in 48kHz

template <typename T>
class CircularQueue
{
private:
    std::unique_ptr<T[]> raw_buffer_;
    std::span<T> buffer_;
    size_t size_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;

    static bool is_power_of_two(size_t n)
    {
        return (n & (n - 1)) == 0;
    }

public:
    explicit CircularQueue(size_t n)
        : raw_buffer_(nullptr), buffer_(), size_(n), head_(0), tail_(0)
    {
        if (!is_power_of_two(n))
        {
            throw std::invalid_argument("Size must be a power of 2");
        }
        raw_buffer_ = std::make_unique<T[]>(n);
        buffer_ = std::span<T>(raw_buffer_.get(), n);
    }

    bool push(const T &item)
    {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next_head = (head + 1) & (size_ - 1);

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
            return std::nullopt; // empty
        }

        T item = buffer_[tail];
        tail_.store((tail + 1) & (size_ - 1), std::memory_order_release);
        return item;
    }

    bool empty() const
    {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    bool full() const
    {
        return ((head_.load(std::memory_order_acquire) + 1) & (size_ - 1)) == tail_.load(std::memory_order_acquire);
    }

    size_t capacity() const
    {
        return size_;
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

    // std::vector<std::vector<unsigned char>> opus_packets;
    CircularQueue<std::vector<unsigned char>> opus_queue(1024);
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

        // opus_packets.emplace_back(opus_data.begin(), opus_data.begin() + encoded_bytes);
        std::vector<unsigned char> packet(opus_data.begin(), opus_data.begin() + encoded_bytes);
        if (!opus_queue.push(packet))
        {
            std::cerr << "Queue is full, dropping packet!" << std::endl;
        }
    }

    opus_encoder_destroy(encoder);

    std::cout << "Dequeuing and printing sizes of encoded packets:" << std::endl;
    while (!opus_queue.empty())
    {
        auto pkt = opus_queue.pop();
        if (pkt)
        {
            std::cout << "Packet size: " << pkt->size() << " bytes" << std::endl;
        }
    }
    return 0;
}

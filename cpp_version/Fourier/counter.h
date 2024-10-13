#ifndef FOURIER_COUNTER_H
#define FOURIER_COUNTER_H

#include "../Utility/headers.h"
#include "record.h"

using namespace std;

namespace Fourier {

    class counter : public abstract_counter {
    protected:
        // meet alignment request
        static_assert((WINDOW * 4) % 16 == 0);
        static_assert(MAX_LENGTH % WINDOW == 0);
        constexpr static const int DEPTH = (FULL_DEPTH * 4) / 6;

        static PFFFT_Setup* setup;
        TIME start_time;
        TIME window_n;
        float* recent; // data in the most recent WINDOW
        float* output; // temp array for transform
        heap<record, DEPTH> history;

        mutable STREAM_QUEUE cache{};

        void transform(const uint16_t start) {
            pffft_transform(setup, recent, output, nullptr, PFFFT_FORWARD);
            for(int i = 0; i < WINDOW; i++)
                history.insert({(uint16_t)(start + i), output[i]});
            memset(recent, 0, WINDOW * 4);
        }
    public:
        counter() : start_time(0), window_n(0), history({}) {
            recent = static_cast<float *>(pffft_aligned_malloc(WINDOW * 4));
            output = static_cast<float *>(pffft_aligned_malloc(WINDOW * 4));
        };
        ~counter() {
            pffft_aligned_free(recent);
            pffft_aligned_free(output);
        }
        counter(const counter& other) {
            start_time = other.start_time;
            window_n = other.window_n;
            recent = static_cast<float *>(pffft_aligned_malloc(WINDOW * 4));
            memcpy(recent, other.recent, WINDOW * 4);
            output = static_cast<float *>(pffft_aligned_malloc(WINDOW * 4));
            history = other.history;
        }
        counter(counter&& other) noexcept {
            start_time = other.start_time;
            window_n = other.window_n;
            recent = exchange(other.recent, nullptr);
            output = exchange(other.output, nullptr);
            history = other.history;
        }
        counter& operator=(const counter& other) {
            if(this == &other)
                return *this;
            start_time = other.start_time;
            window_n = other.window_n;
            memcpy(recent, other.recent, WINDOW * 4);
            history = other.history;
            return *this;
        }
        counter& operator=(counter&& other) noexcept {
            start_time = other.start_time;
            window_n = other.window_n;
            pffft_aligned_free(recent);
            recent = exchange(other.recent, nullptr);
            history = other.history;
            return *this;
        }

        void reset() override {
            start_time = 0;
            window_n = 0;
            memset(recent, 0, WINDOW * 4);
            history.reset();
            cache.clear();
        }

        bool count(TIME t, HASH, DATA c) override {
            assert(t >= start_time);
            if(start_time == 0) [[unlikely]] {
                start_time = t;
            } else if(t - start_time >= MAX_LENGTH) [[unlikely]] {
                flush();
                return true;
            } else if((t - start_time) / WINDOW > window_n) [[unlikely]] {
                flush();
                window_n = (t - start_time) / WINDOW;
            }
            recent[(t - start_time) % WINDOW] += c;
            return false;
        }

        void flush() override {
            if(empty())
                return;
            transform(window_n * WINDOW);
        }

        bool empty() const override {
            return start_time == 0;
        }

        TIME start() const override {
            return start_time;
        }

        STREAM_QUEUE rebuild(HASH) const override {
            assert(!empty());
            if(!cache.empty())
                return cache;

            cache.resize(MAX_LENGTH);
            static auto* origin = static_cast<float *>(pffft_aligned_malloc(MAX_LENGTH * 4));
            static auto* buffer = static_cast<float *>(pffft_aligned_malloc(MAX_LENGTH * 4));
            static auto* worker = static_cast<float *>(pffft_aligned_malloc(WINDOW * 4));

            memset(origin, 0, MAX_LENGTH * 4);

            for(int i = 0; i < history.size; i++) {
                uint16_t pos = history.heap_data[i].pos;
                origin[pos] = history.heap_data[i].data;
            }
            for(int i = 0; i < MAX_LENGTH / WINDOW; i++) {
                pffft_transform(setup, origin + i * WINDOW, buffer + i * WINDOW, worker, PFFFT_BACKWARD);
            }

            for(int i = 0; i < MAX_LENGTH; i++) {
                cache[i].first = start_time + i;
                cache[i].second = (buffer[i] / WINDOW) >= 0 ? (buffer[i] / WINDOW) : 0;
            }

            //pffft_aligned_free(origin);
            //pffft_aligned_free(buffer);

            return cache;
        }

        size_t serialize() const override {
            size_t result = 0;
            result += sizeof(start_time);
            result += history.serialize();
            return result;
        }
    };

    PFFFT_Setup* counter::setup = pffft_new_setup(WINDOW, PFFFT_REAL);

} // Fourier

#endif //FOURIER_COUNTER_H

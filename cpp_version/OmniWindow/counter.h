#ifndef OMNIWINDOW_COUNTER_H
#define OMNIWINDOW_COUNTER_H

#include "../Utility/headers.h"

using namespace std;

namespace OmniWindow {

    class counter : public abstract_counter {
    protected:
        constexpr static const int DEPTH = FULL_DEPTH;
        constexpr static const int RATE = MAX_LENGTH / DEPTH;
        TIME start_time{};
        array<DATA, DEPTH + (DEPTH * RATE < MAX_LENGTH ? 1 : 0)> history{};

        mutable STREAM_QUEUE cache{};
    public:
        void reset() override {
            start_time = 0;
            history.fill(0);
            cache.clear();
        }

        bool count(TIME t, HASH, DATA c) override {
            assert(t >= start_time);
            if(start_time == 0) [[unlikely]] {
                start_time = t;
            } else if(t - start_time >= MAX_LENGTH) [[unlikely]] {
                return true;
            }
            auto index = (t - start_time) / RATE;
            history[index] += c;
            return false;
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
            int pos = 0;
            for(; pos < DEPTH * RATE; pos++) {
                cache[pos].first = start_time + pos;
                cache[pos].second = history[pos / RATE] / RATE;
                assert(cache[pos].second >= 0);
            }
            for(; pos < MAX_LENGTH; pos++) {
                cache[pos].first = start_time + pos;
                assert(MAX_LENGTH - DEPTH * RATE > 0);
                cache[pos].second = history[pos / RATE] / (MAX_LENGTH - DEPTH * RATE);
                assert(cache[pos].second >= 0);
            }
            return cache;
        }

        size_t serialize() const override {
            size_t result = 0;
            result += sizeof(start_time);
            // history has fixed length
            for(auto& d : history)
                result += sizeof(d);
            return result;
        }
    };

} // OmniWindow

#endif //OMNIWINDOW_COUNTER_H

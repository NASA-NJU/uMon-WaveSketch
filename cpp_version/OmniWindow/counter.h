#ifndef OMNIWINDOW_COUNTER_H
#define OMNIWINDOW_COUNTER_H

#include "../Utility/headers.h"

using namespace std;

namespace OmniWindow {

    class counter : public abstract_counter {
    protected:
        constexpr static const int DEPTH = FULL_DEPTH * 2;
        constexpr static const int RATE = MAX_LENGTH / DEPTH;
        TIME start_time{};
        array<DATA, DEPTH> history{};

        mutable STREAM_QUEUE cache{};
    public:
        void reset() override {
            start_time = 0;
            history.fill(0);
            cache.clear();
        }

        bool count(TIME t, HASH) override {
            assert(t >= start_time);
            if(start_time == 0) [[unlikely]] {
                start_time = t;
            } else if(t - start_time >= MAX_LENGTH) [[unlikely]] {
                return true;
            }
            auto index = (t - start_time) / RATE;
            history[index]++;
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
            for(int pos = 0; pos < MAX_LENGTH; pos++) {
                cache[pos].first = start_time + pos;
                cache[pos].second = history[pos / RATE] / RATE;
                assert(cache[pos].second >= 0);
            }
            return cache;
        }
    };

} // OmniWindow

#endif //OMNIWINDOW_COUNTER_H

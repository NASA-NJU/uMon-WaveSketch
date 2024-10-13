#ifndef WAVELET_INTERVAL_H
#define WAVELET_INTERVAL_H

#include "../Utility/headers.h"

using namespace std;

namespace Wavelet {
    
    typedef uint16_t TIME_DIFF;

    // store by (p, d) pair, example logic: (_ => empty time slot; p => period; d => delta)
    //   start       d = 5       d = 0      d = 7 last
    //     ^         |---|         :       |-----| ^
    //     ++++++++++______++++++++_+++++++________+
    //      |-------|       |-----|  |----|        :
    //        p = 9          p = 7   p = 6       p = 0
    //     [--------|-----][------|][-----|-------]#
    //   pairs:   (9,5)        (7,0)    (6,7)    p = 0
    // rebuild example: (start from last)
    //                                             v => t = last
    //                                            v => t--
    //                         ...+_+++++++________+
    //                                    ^ => t -= d+1
    //                             ^ => t-- for p+1 times
    //                            v =>t -= d+1
    //                    v => t-- for p+1 times
    //     ++++++++++______++++++++_...
    //              ^ => t-= d+1
    //    ^ => t-- for p+1 times
    class interval : public abstract_counter {
    protected:
        constexpr static const int LENGTH = MAX_LENGTH / 2;
        TIME start_time{};
        TIME last_time{};
        // consecutive time period
        TIME_DIFF period{};
        uint16_t pointer{};

        array<pair<TIME_DIFF, TIME_DIFF>, LENGTH> history{};

        mutable STREAM_QUEUE cache{};
    public:
        void reset() override {
            start_time = 0;
            last_time = 0;
            period = 0;
            pointer = 0;
            cache.clear();
        }

        bool same_as_last(TIME t) {
            if(start_time == 0) [[unlikely]] {
                start_time = last_time = t;
            }
            return t == last_time;
        }

        bool count(TIME t, HASH, DATA) override {
            assert(t > last_time);
            if(start_time == 0) [[unlikely]] {
                start_time = t;
            } else if(t - start_time >= MAX_LENGTH) [[unlikely]] {
                return true;
            } else {
                TIME_DIFF delta = t - last_time;
                if(delta != 1) [[unlikely]] {
                    if(pointer == LENGTH) [[unlikely]] {
                        return true;
                    }
                    history[pointer] = {period, delta - 2};
                    period = 0;
                    pointer++;
                } else {
                    period++;
                }
            }
            last_time = t;
            return false;
        }

        STREAM_QUEUE rebuild(HASH) const override {
            if(start_time == 0) [[unlikely]] {
                return {};
            } else if(!cache.empty())
                return cache;

            // rebuild in reverse order
            TIME t = last_time;
            for(int i = 0; i <= period; i++)
                cache.emplace_front(t--, 0);
            for(int pos = pointer - 1; pos >= 0; pos--) {
                t -= history[pos].second + 1;
                for(int i = 0; i <= history[pos].first; i++)
                    cache.emplace_front(t--, 0);
            }

            return cache;
        }

        bool empty() const override {
            return start_time == 0;
        }

        TIME start() const override {
            return start_time;
        }

        size_t serialize() const override {
            size_t result = 0;
            // test if start_time == 0
            result += sizeof(bool);
            result += sizeof(last_time);
            result += sizeof(period);
            result += sizeof(pointer);
            for(int i = 0; i < pointer; i++) {
                result += sizeof(history[i].first);
                result += sizeof(history[i].second);
            }
            return result;
        }
    };

} // Wavelet

#endif //WAVELET_INTERVAL_H

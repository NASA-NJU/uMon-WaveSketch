#ifndef NAIVE_CMS_COUNTER_H
#define NAIVE_CMS_COUNTER_H

#include "../Utility/headers.h"

using namespace std;

namespace NaiveCMS {

    class counter : public abstract_counter {
    protected:
        constexpr static const int DEPTH = FULL_DEPTH;
        TIME start_time{};
        array<DATA, DEPTH> history{};
    public:
        void reset() override {
            start_time = 0;
            history.fill(0);
        }

        bool count(TIME t, HASH h, DATA c) override {
            assert(t >= start_time);
            if(start_time == 0) [[unlikely]] {
                start_time = t;
            } else if(t - start_time >= MAX_LENGTH) [[unlikely]] {
                return true;
            }
            history[h % DEPTH] += c;
            return false;
        }

        bool empty() const override {
            return start_time == 0;
        }

        TIME start() const override {
            return start_time;
        }

        STREAM_QUEUE rebuild(HASH h) const override { // TODO: fix this
            cerr << "Should not reach here." << endl;
            assert(false);
            return {};
        }

        DATA query(HASH h) const {
            return history[h % DEPTH];
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

} // NaiveCMS

#endif //NAIVE_CMS_COUNTER_H

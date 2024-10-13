#ifndef PERSIST_AMS_COUNTER_H
#define PERSIST_AMS_COUNTER_H

#include "../Utility/headers.h"

using namespace std;

namespace PersistAMS {

    class record : public STREAM_QUEUE::value_type {
    public:
        using Base = STREAM_QUEUE::value_type;
        using Base::Base;

        size_t serialize() const {
            size_t result = 0;
            result += sizeof(Base::first_type);
            result += sizeof(Base::second_type);
            return result;
        }
    };

    class counter : public abstract_counter {
    protected:
        // random generator
        constexpr static const int DELTA = MAX_LENGTH / ROUND(FULL_DEPTH * 4 + 4 - 24, 10);
        static mt19937 gen;
        static uniform_int_distribution<> dis;
        static bool test() {
            return dis(gen) == 1;
        }

        TIME start_time{};
        TIME last_time[2]{};
        DATA value[2]{};
        list<record> history[2]{};

        mutable STREAM_QUEUE cache{};
    public:
        void reset() override {
            start_time = 0;
            last_time[0] = 0;
            last_time[1] = 0;
            value[0] = 0;
            value[1] = 0;
            history[0].clear();
            history[1].clear();
            cache.clear();
        }

        bool count(TIME t, HASH h, DATA c) override {
            HASH sign = h % 2;
            assert(t >= last_time[sign]);
            if(start_time == 0) [[unlikely]] {
                start_time = last_time[0] = last_time[1] = t;
            } else if(t - start_time >= MAX_LENGTH) [[unlikely]] {
                flush();
                return true;
            }

            if(t > last_time[sign])
                if(test())
                    history[sign].emplace_back(last_time[sign], value[sign]);

            last_time[sign] = t;
            value[sign] += c;
            return false;
        }

        void flush() override {
            if(empty())
                return;
            if(test())
                history[0].emplace_back(last_time[0], value[0]);
            if(test())
                history[1].emplace_back(last_time[1], value[1]);
        }

        bool empty() const override {
            return start_time == 0;
        }

        TIME start() const override {
            return start_time;
        }

        STREAM_QUEUE rebuild(HASH h) const override {
            assert(!empty());
            DATA sign = h % 2 ? 1 : -1;
            if(cache.empty()) {
                cache.resize(MAX_LENGTH);
                DATA last_v0 = 0;
                DATA last_v1 = 0;
                auto p0 = history[0].begin();
                auto p1 = history[1].begin();
                for(int pos = 0; pos < MAX_LENGTH; pos++) {
                    TIME t = start_time + pos;
                    DATA v = 0;
                    if(p0 != history[0].end() && t >= p0->first) {
                        v += p0->second - last_v0;
                        last_v0 = p0->second;
                        p0++;
                    }
                    if(p1 != history[1].end() && t >= p1->first) {
                        v -= p1->second - last_v1;
                        last_v1 = p1->second;
                        p1++;
                    }
                    cache[pos].first = t;
                    cache[pos].second = v;
                }
            }

            STREAM_QUEUE result = cache;
            for(auto& p : result)
                p.second = sign * p.second >= 0 ? sign * p.second : 0;
            return result;
        }

        size_t serialize() const override {
            size_t result = 0;
            result += sizeof(start_time);
            // counter for histories
            result += sizeof(uint16_t) * 2;
            for(auto& r : history[0])
                result += r.serialize();
            for(auto& r : history[1])
                result += r.serialize();
            return result;
        }
    };

    mt19937 counter::gen(0xAEABDC85);
    uniform_int_distribution<> counter::dis(1, DELTA);

} // PersistAMS

#endif //PERSIST_AMS_COUNTER_H

#ifndef PERSIST_CMS_COUNTER_H
#define PERSIST_CMS_COUNTER_H

#include "../Utility/headers.h"
#include "polygon.h"

using namespace std;

namespace PersistCMS {

    class counter : public abstract_counter {
    protected:
#ifdef BY_BYTES
        constexpr static const int DELTA = PCMS_DELTA * 1024;
#else
        constexpr static const int DELTA = PCMS_DELTA;
#endif

        TIME start_time{};
        TIME last_time{};
        DATA value{};

        list<line> history{};
        polygon solver{};

        mutable STREAM_QUEUE cache{};
    public:
        void reset() override {
            start_time = 0;
            last_time = 0;
            value = 0;
            history.clear();
            solver.reset();
            cache.clear();
        }

        bool count(TIME t, HASH, DATA c) override {
            assert(t >= last_time);
            if(start_time == 0) [[unlikely]] {
                start_time = last_time = t;
            } else if(t - start_time >= MAX_LENGTH) [[unlikely]] {
                flush();
                return true;
            }
            if(t > last_time) {
                line result = solver.insert(last_time, value, DELTA);
                if(result.first != 0)
                    history.push_back(result);
            }
            last_time = t;
            value += c;
            return false;
        }

        void flush() override {
            if(empty())
                return;
            // the last value will not change by now, feed to solver
            line result = solver.insert(last_time, value, DELTA);
            // solver outputs a line => the last value cannot fit in that line => insert 2 lines
            // no output from solver => the last value fits in the recent line => insert 1 line
            if(result.first != 0)
                history.push_back(result);
            history.push_back(solver.current());
        }

        static constexpr DATA evaluate(const node n, const TIME t) {
            auto result = lround(n.second + n.first * t);
            return result >= 0 ? result : 0;
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

            // reconstruct from history
            TIME t = start_time;
            TIME last_t = history.back().first;
            DATA last_d = 0;
            cache.resize(last_t - t + 1);

            for(auto& p : history) {
                for(; t <= p.first; t++) {
                    DATA d = evaluate(p.second, t);
                    cache[t - start_time].first = t;
                    cache[t - start_time].second = d - last_d >= 0 ? d - last_d : 0;
                    last_d = d;
                }
            }

            return cache;
        }

        size_t serialize() const override {
            size_t result = 0;
            result += sizeof(start_time);
            // counter for history
            result += sizeof(uint16_t);
            for(auto& l : history)
                result += l.serialize();
            return result;
        }
    };

} // PersistCMS

#endif //PERSIST_CMS_COUNTER_H

#ifndef PERSIST_CMS_COUNTER_H
#define PERSIST_CMS_COUNTER_H

#include "../Utility/headers.h"
#include "polygon.h"

using namespace std;

namespace PersistCMS {

    class counter : public abstract_counter {
    protected:
        constexpr static const int DELTA = 40;

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

        bool count(TIME t, HASH) override {
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
            value++;
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
            DATA last_d = 0;
            for(auto& p : history) {
                for(; t <= p.first; t++) {
                    DATA d = evaluate(p.second, t);
                    DATA diff = d - last_d >= 0 ? d - last_d : 0;
                    cache.emplace_back(t, diff);
                    last_d = d;
                }
            }

            assert(is_sorted(cache.begin(), cache.end()));
            auto q = unique(cache.begin(), cache.end());
            assert(q == cache.end());

            return cache;
        }

    };

} // PersistCMS

#endif //PERSIST_CMS_COUNTER_H

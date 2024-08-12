#ifndef NAIVE_CMS_TABLE_H
#define NAIVE_CMS_TABLE_H

#include "../Utility/headers.h"
#include "counter.h"
#include "key.h"

using namespace std;

namespace NaiveCMS {

    class table : public basic_table<counter> {
    protected:
        TIME start_time{};
        TIME last_time{};

        void derived_reset() override {
            start_time = 0;
            last_time = 0;
        }
    public:
        bool count(const five_tuple& f, TIME t) override {
            if(start_time == 0) [[unlikely]] {
                start_time = t;
            }

            last_time = t;
            key k{f, t};
            for(int row = 0; row < HEIGHT; row++) {
                HASH h = k.hash(seeds[row]);
                HASH rem = h % WIDTH;
                HASH quo = h / WIDTH;
                bool result = counters[row][rem].count(t, quo);
                if(result) {
                    history[row][rem].push_back(counters[row][rem]);
                    counters[row][rem].reset();
                    counters[row][rem].count(t, quo);
                }
            }

            return true;
        }

        STREAM_QUEUE rebuild(const five_tuple& f, TIME start, TIME last) const {
            map<TIME, vector<DATA>> merger;
            for(TIME t = start; t <= last; t++) {
                key k{f, t};
                for(int row = 0; row < HEIGHT; row++) {
                    HASH h = k.hash(seeds[row]);
                    HASH rem = h % WIDTH;
                    HASH quo = h / WIDTH;
                    auto& hc = history[row][rem];
                    auto c = first_history(hc, t);
                    if(c != hc.end() && t >= c->start())
                        merger[t].emplace_back(c->query(quo));
                }
            }

            STREAM_QUEUE result;
            for(auto& p : merger)
                result.emplace_back(p.first, select_val(p.second));

            return result;
        }
    };

} // NaiveCMS

#endif //NAIVE_CMS_TABLE_H

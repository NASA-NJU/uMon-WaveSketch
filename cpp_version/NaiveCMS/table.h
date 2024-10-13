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
        bool count(const five_tuple& f, TIME t, DATA c) override {
            if(start_time == 0) [[unlikely]] {
                start_time = t;
            }

            last_time = t;
            key k{f, t};
            for(int row = 0; row < HEIGHT; row++) {
                HASH h = k.hash(seeds[row]);
                HASH rem = h % WIDTH;
                HASH quo = h / WIDTH;
                bool result = counters[row][rem].count(t, quo, c);
                if(result) {
                    history[row][rem].push_back(counters[row][rem]);
                    counters[row][rem].reset();
                    counters[row][rem].count(t, quo, c);
                }
            }

            return true;
        }

        STREAM_QUEUE rebuild(const five_tuple& f, TIME start, TIME last) const {
            vector<array<DATA, HEIGHT>> merger(last - start + 1);
            STREAM_QUEUE result(last - start + 1);

            for(TIME t = start; t <= last; t++) {
                key k{f, t};
                auto& slot = merger[t - start];
                for(int row = 0; row < HEIGHT; row++) {
                    HASH h = k.hash(seeds[row]);
                    HASH rem = h % WIDTH;
                    HASH quo = h / WIDTH;
                    auto& hc = history[row][rem];
                    auto c = first_history(hc, t);
                    if(c != hc.end() && t >= c->start())
                        slot[row] = c->query(quo);
                    else
                        slot[row] = 0;
                }

                DATA min = select_val(slot);
                assert(min >= 0);
                result[t - start] = make_pair(t, min);
            }

            return result;
        }
    };

} // NaiveCMS

#endif //NAIVE_CMS_TABLE_H

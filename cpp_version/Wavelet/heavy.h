#ifndef WAVELET_HEAVY_H
#define WAVELET_HEAVY_H

#include "../Utility/headers.h"
#include "counter.h"

using namespace std;

namespace Wavelet {

    template<bool BY_THRESHOLD = false>
    class heavy : public basic_table<counter<BY_THRESHOLD>, HALF_WIDTH, PAIR_HEIGHT> {
    protected:
        constexpr static const HASH seed = heavy::seeds[heavy::HEIGHT];
        static_assert(sizeof(heavy::seeds) / sizeof(HASH) >= heavy::HEIGHT + 1);

        uint32_t frequency[heavy::HEIGHT][heavy::WIDTH]{};
        five_tuple label[heavy::HEIGHT][heavy::WIDTH]{};
        deque<five_tuple> history_label[heavy::HEIGHT][heavy::WIDTH]{};

        void derived_reset() override {
            memset(frequency, 0, sizeof(frequency));
            memset(label, 0, sizeof(label));
            for(auto& row : history_label)
                for(auto& c : row)
                    c.clear();
        }
        void save_counter(HASH row, HASH col) override {
            auto& c = heavy::counters[row][col];
            auto& hc = heavy::history[row][col];
            auto& l = label[row][col];
            auto& hl = history_label[row][col];

            c.flush();
            hc.push_back(c);
            c.reset();
            hl.push_back(l);
        }
        void evict(HASH row, HASH col) {
            auto& c = heavy::counters[row][col];
            if(c.get_count() >= RETAIN_THRESH)
                save_counter(row, col);
            else
                c.reset();
        }
    public:
        bool count(const five_tuple& f, TIME t, DATA c) override {
            HASH h = f.hash(seed);
            HASH rem = h % heavy::WIDTH;
            HASH quo = h / heavy::WIDTH;
            HASH row;
            // search f in existing labels
            for(row = 0; row < heavy::HEIGHT; row++)
                if(label[row][rem] == f)
                    break;

            if(row == heavy::HEIGHT) {
                // five-tuple not found
                for(row = 0; row < heavy::HEIGHT; row++) {
                    frequency[row][rem]--;
                    if(frequency[row][rem] == 0) [[unlikely]] {
                        // eviction happens
                        evict(row, rem);
                        label[row][rem] = f;
                        break;
                    }
                }
            }
            if(row == heavy::HEIGHT) {
                // insertion failed
                return false;
            } else {
                // insertion successful
                frequency[row][rem] += HIT_RATIO;
                bool result = heavy::counters[row][rem].count(t, quo, c);
                if(result) {
                    save_counter(row, rem);
                    heavy::counters[row][rem].count(t, quo, c);
                }
                return true;
            }
        }

        STREAM_QUEUE rebuild(const five_tuple& f, TIME, TIME) const override {
            map<TIME, DATA> merger;
            // search f in existing labels
            HASH col = f.hash(seed) % heavy::WIDTH;
            HASH row;
            for(row = 0; row < heavy::HEIGHT; row++) {
                auto& hl = history_label[row][col];
                for(auto l = hl.begin(); l != hl.end(); l++) {
                    if(*l == f) {
                        auto& hc = heavy::history[row][col];
                        auto c = hc.begin() + (l - hl.begin());
                        for(auto& p : c->rebuild(row))
                            merger[p.first] = p.second;
                    }
                }
            }

            STREAM_QUEUE result;
            for(auto& p : merger)
                result.emplace_back(p.first, p.second);

            return result;
        }

        LABELS labels() const {
            LABELS result;
            for(auto& row : history_label)
                for(auto& c : row)
                    result.insert(c);
            return result;
        }

        void list_min(vector<record>& result) const {
            for(int row = 0; row < heavy::HEIGHT; row++)
                for(int col = 0; col < heavy::WIDTH; col++)
                    for(auto& c : heavy::history[row][col])
                        if(c.heap_full())
                            result.push_back(c.list_min());
        }
    };

} // Wavelet

#endif //WAVELET_HEAVY_H

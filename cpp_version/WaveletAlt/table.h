#ifndef WAVELET_ALT_TABLE_H
#define WAVELET_ALT_TABLE_H

#include "../Utility/headers.h"
#include "counter.h"

using namespace std;

namespace WaveletAlt {

    template<unsigned QUEUE_N = 1>
    class table : public basic_table<counter<QUEUE_N>, HALF_WIDTH, FULL_HEIGHT> {
    protected:
        DATA select_val(vector<DATA>& vals) const override {
            return table::select_median(vals);
        }
    public:
        // for every flow from heavy-hitter table, subtract its value from the corresponding counter
        void subtract(const STREAM& dict) const {
            for(auto& p : dict) {
                for(int row = 0; row < table::HEIGHT; row++) {
                    auto& f = p.first;
                    auto q_begin = p.second.begin();
                    auto q_end = p.second.end();
                    assert(q_begin != q_end);
                    HASH h = f.hash(table::seeds[row]);
                    HASH rem = h % table::WIDTH;
                    HASH quo = h / table::WIDTH;
                    auto& hc = table::history[row][rem];
                    for(auto c = table::first_history(hc, q_begin->first); c != hc.end(); c++) {
                        q_begin = c->subtract(quo, q_begin, q_end);
                        if(q_begin == q_end) [[unlikely]]
                            break;
                    }
                }
            }
        }
    };

} // WaveletAlt

#endif //WAVELET_ALT_TABLE_H

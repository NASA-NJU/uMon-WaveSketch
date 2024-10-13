#ifndef WAVELET_TABLE_H
#define WAVELET_TABLE_H

#include "../Utility/headers.h"
#include "counter.h"

using namespace std;

namespace Wavelet {

    template<bool BY_THRESHOLD = false>
    class table : public basic_table<counter<BY_THRESHOLD>, FULL_WIDTH, LESS_HEIGHT> {
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
                        if(q_begin == q_end) [[unlikely]]
                            break;
                        q_begin = c->subtract(quo, q_begin, q_end);
                    }
                }
            }
        }

        void list_min(vector<record>& result) const {
            for(int row = 0; row < table::HEIGHT; row++)
                for(int col = 0; col < table::WIDTH; col++)
                    for(auto& c : table::history[row][col])
                        if(c.heap_full())
                            result.push_back(c.list_min());
        }
    };

} // Wavelet

#endif //WAVELET_TABLE_H

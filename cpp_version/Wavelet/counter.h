#ifndef WAVELET_COUNTER_H
#define WAVELET_COUNTER_H

#include "../Utility/headers.h"
#include "interval.h"
#include "record.h"

using namespace std;

namespace Wavelet {

    template<unsigned QUEUE_N = 1>
    class counter : public abstract_counter {
    protected:
        constexpr static const unsigned AGGREGATE = 1;
        constexpr static const int DEPTH = FULL_DEPTH / QUEUE_N;

        // # of data read
        uint16_t read_count{};
        array<DATA, AGGREGATE> value{};

        array<DATA, LEVEL> last_coef{};
        array<DATA, RESERVED> top_level{};

        heap<record, DEPTH> detail[QUEUE_N]{};

        interval time{};

        mutable STREAM_QUEUE cache{};

        DATA max_value() const {
            return *max_element(value.begin(), value.end());
        }
        void heap_insert(uint8_t level, DATA d) {
            uint16_t pos = (read_count >> level) << level;
            record last(pos, d);

            detail[level % QUEUE_N].insert(last);
        }
        DATA transform_pair(uint8_t level, DATA d) {
            DATA lo = last_coef[level] + d;
            DATA hi = last_coef[level] - d;

            heap_insert(level, hi);
            return lo;
        }
        static void inverse_transform(DATA& lo, DATA& hi) {
            DATA l = (lo + hi) / 2;
            DATA h = (lo - hi) / 2;
            lo = l;
            hi = h;
        }
    public:
        uint16_t get_count() const {
            return read_count;
        }
        void reset() override {
            read_count = 0;
            value.fill(0);
            for(auto& d : detail)
                d.reset();
            time.reset();
            cache.clear();
        }

        bool count(TIME t, HASH h) override {
            h %= AGGREGATE;
            if(time.same_as_last(t)) {
                value[h]++;
                return false;
            }

            if(time.count(t, h)) [[unlikely]] {
                flush();
                return true;
            }

            flush();
            value[h]++;
            return false;
        }

        void flush() override {
            if(empty())
                return;

            int level = countr_one(read_count & INDEX_MASK);
            DATA last_val = max_value();
            for(int l = 0; l < level; l++)
                last_val = transform_pair(l, last_val);

            if(level < LEVEL)
                last_coef[level] = last_val;
            else
                top_level[read_count >> LEVEL] = last_val;

            read_count++;
            value.fill(0);
        }

        STREAM_QUEUE rebuild(HASH) const override {
            // parameter has no use here
            assert(!empty());
            if(!cache.empty())
                return cache;

            cache = time.rebuild({});

            vector<DATA> temp(read_count, 0);
            // copy heap data
            for(auto& d : detail)
                for(int i = 0; i < d.size; i++) {
                    int pos = d.heap_data[i].pos;
                    temp[pos] = d.heap_data[i].data();
                }

            // copy top level
            for(int i = 0; i < read_count >> LEVEL; i++)
                temp[i << LEVEL] = top_level[i];

            // copy data yet to be transformed
            bitset<16> mask{read_count};
            for(int i = 0; i < LEVEL; i++)
                if(mask[i])
                    temp[(read_count >> (i + 1)) << (i + 1)] = last_coef[i];

            // inverse-transform each section except for the last one
            uint16_t last_section = (read_count >> LEVEL) << LEVEL;
            for(uint32_t frag = 0; frag < last_section; frag += 1 << LEVEL) {
                for(uint32_t p = 1 << LEVEL; p > 0; p--) {
                    uint32_t pos = frag + p;
                    for(int i = countr_zero(p) - 1; i >= 0; i--) {
                        inverse_transform(temp[pos - (2 << i)], temp[pos - (1 << i)]);
                    }
                }
            }

            // inverse-transform the last section
            for(uint32_t pos = read_count; pos > last_section; pos--)
                for(int i = countr_zero(pos) - 1; i >= 0; i--)
                    inverse_transform(temp[pos - (2 << i)], temp[pos - (1 << i)]);

            // copy from temp to result
            for(int pos = 0; pos < cache.size(); pos++)
                cache[pos].second = temp[pos] > 0 ? temp[pos] : 1;

            return cache;
        }

        bool empty() const override {
            return time.empty();
        }

        TIME start() const override {
            return time.start();
        }
    };

} // Wavelet

#endif //WAVELET_COUNTER_H

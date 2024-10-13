#ifndef WAVELET_ALT_COUNTER_H
#define WAVELET_ALT_COUNTER_H

#include "../Utility/headers.h"
#include "interval.h"
#include "record.h"

using namespace std;

namespace WaveletAlt {

    typedef STREAM_QUEUE::const_iterator SQptr;

    template<unsigned QUEUE_N = 1>
    class counter : public abstract_counter {
    protected:
        constexpr static const int DEPTH = ROUND(FULL_DEPTH * 4 + 4 - 42, 4) / QUEUE_N;
        // # of data read
        uint16_t read_count{};
        DATA value{};

        array<DATA, LEVEL> last_coef{};
        array<DATA, RESERVED> top_level{};

        heap<record, DEPTH> detail[QUEUE_N]{};

        interval time{};

        mutable STREAM_QUEUE cache{};

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
            value = 0;
            for(auto& d : detail)
                d.reset();
            time.reset();
            cache.clear();
        }

        bool count(TIME t, HASH h, DATA c) override {
            DATA sign = h % 2 ? c : -c;
            if(time.same_as_last(t)) {
                value += sign;
                return false;
            }

            if(time.count(t, h, c)) [[unlikely]] {
                flush();
                return true;
            }

            flush();
            value += sign;
            return false;
        }

        void flush() override {
            if(empty())
                return;

            int level = countr_one(read_count & INDEX_MASK);
            DATA last_val = value;
            for(int l = 0; l < level; l++)
                last_val = transform_pair(l, last_val);

            if(level < LEVEL)
                last_coef[level] = last_val;
            else
                top_level[read_count >> LEVEL] = last_val;

            read_count++;
            value = 0;
        }

        STREAM_QUEUE rebuild(HASH h) const override {
            assert(!empty());
            DATA sign = h % 2 ? 1 : -1;
            if(cache.empty()) {
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
                    if (mask[i])
                        temp[(read_count >> (i + 1)) << (i + 1)] = last_coef[i];

                // inverse-transform each section except for the last one
                uint16_t last_section = (read_count >> LEVEL) << LEVEL;
                for(uint32_t frag = 0; frag < last_section; frag += 1 << LEVEL) {
                    for (uint32_t p = 1 << LEVEL; p > 0; p--) {
                        uint32_t pos = frag + p;
                        for (int i = countr_zero(p) - 1; i >= 0; i--) {
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
            }

            STREAM_QUEUE result = cache;
            for(auto& p : result)
                p.second = sign * p.second > 0 ? sign * p.second : 1;
            return result;
        }

        // given a precisely-recorded flow, subtract its value from every recorded time-window
        SQptr subtract(HASH h, SQptr it, const SQptr& end) const {
            assert(!empty());
            if(cache.empty()) [[unlikely]] {
                rebuild(h);
            }

            DATA sign = h % 2 ? 1 : -1;
            auto cache_it = upper_bound(cache.begin(), cache.end(), it->first,
                                        [](const TIME& t, const auto& p) { return t <= p.first; });

            while(it != end && cache_it != cache.end()) {
                if(it->first > cache_it->first)
                    cache_it++;
                else if(it->first < cache_it->first)
                    it++;
                else [[likely]] {
                    cache_it->second -= sign * it->second;
                    it++;
                    cache_it++;
                }
            }

            return it;
        }

        bool empty() const override {
            return time.empty();
        }

        TIME start() const override {
            return time.start();
        }
    };

} // WaveletAlt

#endif //WAVELET_ALT_COUNTER_H

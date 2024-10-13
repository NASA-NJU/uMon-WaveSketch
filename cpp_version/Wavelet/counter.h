#ifndef WAVELET_COUNTER_H
#define WAVELET_COUNTER_H

#include "../Utility/headers.h"
#include "interval.h"
#include "record.h"

using namespace std;

namespace Wavelet {

    typedef STREAM_QUEUE::const_iterator SQptr;
    typedef uint16_t DATA16;

    template<bool BY_THRESHOLD = false>
    class counter : public abstract_counter {
    protected:
#ifdef BY_BYTES
        constexpr static const int SCALE = 1000;
#else
        constexpr static const int SCALE = 1;
#endif
        constexpr static const int T_DEPTH = ROUND(FULL_DEPTH * 4 + 4 - 44, 4) / 2; // threshold
        constexpr static const int DEPTH = ROUND(FULL_DEPTH * 4 + 4 - 42, 4); // priority

        // # of data read
        TIME start_time{};
        TIME_DIFF elapse{};
        DATA16 value{};

        array<DATA16, LEVEL> last_coef{};
        array<DATA16, RESERVED> top_level{};

        heap<record, DEPTH> detail{};
        pseudo_heap<record, T_DEPTH> th_detail[2]{};

        mutable STREAM_QUEUE cache{};

        static DATA truncate(DATA d) {
            return d / SCALE + (SCALE > 1 && d % SCALE >= SCALE / 2);
        }
        static DATA recover(DATA t) {
            return t * SCALE;
        }
        static DATA16 adds(DATA16 a, DATA16 b) {
            DATA16 r = a + b;
            if(r < a)
                r = -1;
            return r;
            return r > a ? r : -1;
        }

        void heap_insert(uint8_t level, DATA d) {
            uint16_t pos = (elapse >> level) << level;
            record last(pos, d);

            if(d != 0) {
                if(BY_THRESHOLD)
                    th_detail[level % 2].insert(last);
                else
                    detail.insert(last);
            }
        }
        DATA transform_pair(uint8_t level, DATA d) {
            DATA lo = adds(last_coef[level], d);
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
            return elapse;
        }
        void reset() override {
            start_time = 0;
            elapse = 0;
            value = 0;
            top_level.fill(0);

            if(BY_THRESHOLD) {
                th_detail[0].reset();
                th_detail[1].reset();
            } else
                detail.reset();

            cache.clear();
        }

        bool count(TIME t, HASH, DATA c) override {
            assert(t >= start_time);
            if(start_time == 0) [[unlikely]] {
                start_time = t;
            } else if(t - start_time >= MAX_LENGTH) [[unlikely]] {
                flush();
                return true;
            } else if(t > start_time + elapse) [[unlikely]] {
                flush();
                if(t > start_time + elapse) [[unlikely]] {
                    align(t);
                }
            }

            value += c;
            return false;
        }

        void flush() override {
            if(empty())
                return;

            int level = countr_one(elapse & INDEX_MASK);
            DATA last_val = truncate(value);
            for(int l = 0; l < level; l++)
                last_val = transform_pair(l, last_val);

            if(level < LEVEL)
                last_coef[level] = last_val;
            else
                top_level[elapse >> LEVEL] = last_val;

            elapse++;
            value = 0;
        }

        void align(TIME t) {
            if(empty())
                return;

            TIME_DIFF new_elapse = t - start_time;
            int level = 31 - countl_zero((uint32_t)(elapse ^ new_elapse));
            if(level >= LEVEL)
                level = LEVEL;

            bitset<LEVEL> mask_old{elapse};
            bitset<LEVEL> mask_new{new_elapse};

            DATA last_val = 0;
            for(int l = 0; l < level; l++) {
                if(!mask_old[l]) {
                    last_coef[l] = last_val;
                    last_val = 0;
                    elapse += 1 << l;
                }
                last_val = transform_pair(l, last_val);

                if(mask_new[l])
                    last_coef[l] = 0;
            }

            if(level < LEVEL)
                last_coef[level] = last_val;
            else
                top_level[elapse >> LEVEL] = last_val;

            elapse = new_elapse;
            value = 0;
        }

        STREAM_QUEUE rebuild(HASH) const override {
            // parameter has no use here
            assert(!empty());
            if(!cache.empty())
                return cache;

            cache.resize(elapse);

            vector<DATA> temp(elapse, 0);
            // copy heap data
            if(BY_THRESHOLD) {
                for(auto& d : th_detail)
                    for(int i = 0; i < d.size_hi; i++) {
                        int pos = d.heap_data[i].pos;
                        temp[pos] = recover(d.heap_data[i].data());
                    }
                for(auto& d : th_detail)
                    for(int i = T_DEPTH - 1; i > d.size_lo; i--) {
                        int pos = d.heap_data[i].pos;
                        temp[pos] = recover(d.heap_data[i].data());
                    }
            }
            else
                for(int i = 0; i < detail.size; i++) {
                    int pos = detail.heap_data[i].pos;
                    temp[pos] = recover(detail.heap_data[i].data());
                }

            // copy top level
            for(int i = 0; i < elapse >> LEVEL; i++)
                temp[i << LEVEL] = recover(top_level[i]);

            // copy data yet to be transformed
            bitset<16> mask{elapse};
            for(int i = 0; i < LEVEL; i++)
                if(mask[i])
                    temp[(elapse >> (i + 1)) << (i + 1)] = recover(last_coef[i]);

            // inverse-transform each section except for the last one
            uint16_t last_section = (elapse >> LEVEL) << LEVEL;
            for(uint32_t frag = 0; frag < last_section; frag += 1 << LEVEL) {
                for(uint32_t p = 1 << LEVEL; p > 0; p--) {
                    uint32_t pos = frag + p;
                    for(int i = countr_zero(p) - 1; i >= 0; i--) {
                        inverse_transform(temp[pos - (2 << i)], temp[pos - (1 << i)]);
                    }
                }
            }

            // inverse-transform the last section
            for(uint32_t pos = elapse; pos > last_section; pos--)
                for(int i = countr_zero(pos) - 1; i >= 0; i--)
                    inverse_transform(temp[pos - (2 << i)], temp[pos - (1 << i)]);

            // copy from temp to result
            for(int pos = 0; pos < elapse; pos++) {
                cache[pos].first = start_time + pos;
                cache[pos].second = temp[pos] > 0 ? temp[pos] : SCALE;
            }

            return cache;
        }

        // given a precisely-recorded flow, subtract its value from every recorded time-window
        SQptr subtract(HASH h, SQptr it, const SQptr& end) const {
            assert(!empty());
            if(cache.empty()) [[unlikely]] {
                rebuild(h);
            }

            auto cache_it = upper_bound(cache.begin(), cache.end(), it->first,
                                        [](const TIME& t, const auto& p) { return t <= p.first; });
            while(it != end && cache_it != cache.end()) {
                if(it->first > cache_it->first)
                    cache_it++;
                else if(it->first < cache_it->first)
                    it++;
                else [[likely]] {
                    cache_it->second -= it->second;
                    it++;
                    cache_it++;
                }
            }

            return it;
        }

        bool empty() const override {
            return start_time == 0;
        }

        TIME start() const override {
            return start_time;
        }

        size_t serialize() const override {
            size_t result = 0;
            result += sizeof(start_time);
            result += sizeof(elapse);
            result += sizeof(DATA16) * popcount(elapse & INDEX_MASK);
            result += sizeof(DATA16) * min<uint32_t>(RESERVED, elapse >> LEVEL);
            if(BY_THRESHOLD) {
                result += th_detail[0].serialize();
                result += th_detail[1].serialize();
            } else
                result += detail.serialize();
            return result;
        }

        record list_min() const {
            return detail.heap_data[0];
        }

        bool heap_full() const {
            return detail.size == DEPTH;
        }
    };

} // Wavelet

#endif //WAVELET_COUNTER_H

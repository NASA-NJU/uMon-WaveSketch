#ifndef WAVELET_ALT_H
#define WAVELET_ALT_H

#include "../Utility/headers.h"
#include "heavy.h"
#include "table.h"

using namespace std;

template<unsigned QUEUE_N = 1>
class wavelet_alt : public abstract_scheme {
protected:
    WaveletAlt::heavy<QUEUE_N> top{};
    WaveletAlt::table<QUEUE_N> low{};
public:
    void reset() override {
        top.reset();
        low.reset();
    }

    void count(const five_tuple& f, const TIME t) override {
        top.count(f, t);
        low.count(f, t);
    }

    void flush() override {
        top.flush();
        low.flush();
    }

    STREAM rebuild(const STREAM& dict) const override {
        STREAM heavy_dict;
        for(auto& p : dict) {
            auto& f = p.first;
            auto& q = p.second;
            heavy_dict[f] = top.rebuild(f, q.front().first, q.back().first);
        }
        low.subtract(heavy_dict);

        STREAM result;
        for(auto& p : dict) {
            auto& f = p.first;
            auto& q = p.second;
            STREAM_QUEUE q_top = heavy_dict[f];
            STREAM_QUEUE q_low = low.rebuild(f, q.front().first, q.back().first);
            STREAM_QUEUE& q_res = result[f];
            set_union(q_top.begin(), q_top.end(), q_low.begin(), q_low.end(), back_inserter(q_res),
                      [](const pair<TIME, DATA>& l, const pair<TIME, DATA>& r) { return l.first < r.first; });
        }

        return result;
    }
};


#endif //WAVELET_ALT_H

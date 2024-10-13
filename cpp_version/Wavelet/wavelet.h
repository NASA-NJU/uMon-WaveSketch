#ifndef WAVELET_H
#define WAVELET_H

#include "../Utility/headers.h"
#include "heavy.h"
#include "table.h"

using namespace std;

template<bool BY_THRESHOLD = false>
class wavelet : public abstract_scheme {
protected:
    Wavelet::heavy<BY_THRESHOLD> top{};
    Wavelet::table<BY_THRESHOLD> low{};
public:
    void reset() override {
        top.reset();
        low.reset();
    }

    void count(const five_tuple& f, const TIME t, const DATA c) override {
        top.count(f, t, c);
        low.count(f, t, c);
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
            auto temp = top.rebuild(f, q.front().first, q.back().first);
            if(!temp.empty())
                heavy_dict[f] = move(temp);
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

        if(!BY_THRESHOLD)
            set_min();
        return result;
    }

    size_t serialize() const override {
        size_t result = 0;
        result += top.serialize();
        result += low.serialize();
        return result;
    }

    void set_min() const {
        vector<Wavelet::record> result = {};
        top.list_min(result);
        low.list_min(result);
        sort(result.begin(), result.end());
        auto t = result.begin() + result.size() * (SAMPLE_RATE + 10) / 128;
        nth_element(result.begin(), t, result.end());
        auto m = result.begin() + result.size() * 3 / 4;
        nth_element(result.begin(), t, result.end());
        //auto m = max_element(result.begin(), result.end());
        pseudo_heap<Wavelet::record, ROUND(FULL_DEPTH * 4 + 4 - 44, 4) / 2>::thresh_hi = Wavelet::record(992, 36 + SAMPLE_RATE * 2);//*m;
        pseudo_heap<Wavelet::record, ROUND(FULL_DEPTH * 4 + 4 - 44, 4) / 2>::thresh_lo = *t;
    }
};


#endif //WAVELET_H

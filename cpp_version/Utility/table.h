#ifndef TABLE_H
#define TABLE_H

#include <map>

#include "counter.h"

using namespace std;

class abstract_table {
public:
    // reset all related data structures; act as an empty table afterward
    virtual void reset() = 0;
    // return true if inserted successfully
    virtual bool count(const five_tuple& f, TIME t, DATA c) = 0;
    // finish recording and deal with remaining buffered data
    virtual void flush() = 0;
    // rebuild counters of five-tuple f in all possible time-window
    virtual STREAM_QUEUE rebuild(const five_tuple& f, TIME start, TIME last) const = 0;
    // serialize all the non-empty counters in table
    virtual size_t serialize() const = 0;
};

template<DerivedCounter C = abstract_counter, int W = FULL_WIDTH, int H = FULL_HEIGHT>
class basic_table : public abstract_table {
protected:
    constexpr static const HASH seeds[] = {0x5A5A5A5A, 0x42424242, 0xDEADBEEF, 0x12345678};
    constexpr static const int WIDTH = W;
    constexpr static const int HEIGHT = H;
    static_assert(sizeof(seeds) / sizeof(HASH) >= HEIGHT);
    static_assert(HEIGHT > 0);

    C counters[HEIGHT][WIDTH]{};
    deque<C> history[HEIGHT][WIDTH]{};

    virtual void derived_reset() { }
    virtual void save_counter(HASH row, HASH col) {
        history[row][col].push_back(counters[row][col]);
        counters[row][col].reset();
    }
    static auto first_history(const deque<C>& qc, TIME start) {
        return upper_bound(qc.begin(), qc.end(), start,
                             [](const TIME t, const C& c) { return c.start() + MAX_LENGTH > t; });
    }
    virtual DATA select_median(array<DATA, HEIGHT>& vals) const {
        int size = vals.size();
        sort(vals.begin(), vals.end());
        DATA median = 0;
        if(size % 2 == 1)
            median = vals[size / 2];
        else
            median = (vals[size / 2 - 1] + vals[size / 2]) / 2;
        assert(median >= 0);
        if(median < 0)
            median = 0;

        return median;
    }
    virtual DATA select_min(array<DATA, HEIGHT>& vals) const {
        DATA min = *min_element(vals.begin(), vals.end());
        assert(min >= 0);
        if(min < 0)
            min = 0;

        return min;
    }

    virtual DATA select_val(array<DATA, HEIGHT>& vals) const {
        return select_min(vals);
    }
public:
    // reset all related data structures; act as an empty table afterward
    virtual void reset() override {
        derived_reset();
        for(auto& row : counters)
            for(auto& c : row)
                c.reset();
        for(auto& row : history)
            for(auto& c : row)
                c.clear();
    }
    // return true if inserted successfully
    virtual bool count(const five_tuple& f, TIME t, DATA c) override {
        for(int row = 0; row < HEIGHT; row++) {
            HASH h = f.hash(seeds[row]);
            HASH rem = h % WIDTH;
            HASH quo = h / WIDTH;
            bool result = counters[row][rem].count(t, quo, c);
            if(result) {
                save_counter(row, rem);
                counters[row][rem].count(t, quo, c);
            }
        }
        return true;
    }
    // finish recording and deal with remaining buffered data
    virtual void flush() override {
        for(int row = 0; row < HEIGHT; row++) {
            for(int col = 0; col < WIDTH; col++) {
                if(counters[row][col].empty())
                    continue;
                counters[row][col].flush();
                save_counter(row, col);
            }
        }
    }
    // rebuild counters of five-tuple f in [start, last], inclusive
    virtual STREAM_QUEUE rebuild(const five_tuple& f, TIME start, TIME last) const override {
        vector<array<DATA, HEIGHT>> merger(last - start + 1);
        for(auto& a : merger)
            a.fill(0);
        STREAM_QUEUE result(last - start + 1);

        for(int row = 0; row < HEIGHT; row++) {
            HASH h = f.hash(seeds[row]);
            HASH rem = h % WIDTH;
            HASH quo = h / WIDTH;

            auto& hc = history[row][rem];
            for(auto c = first_history(hc, start); c != hc.end(); c++) {
                if(c->start() > last)
                    break;
                for(auto& p : c->rebuild(quo))
                    if(p.first >= start && p.first <= last) [[likely]] {
                        merger[p.first - start][row] = p.second;
                    }
            }
        }

        for(int pos = 0; pos <= last - start; pos++) {
            result[pos].first = pos + start;
            result[pos].second = select_val(merger[pos]);
        }

        return result;
    }
    // serialize all the historic counters
    virtual size_t serialize() const override {
        size_t result = 0;
        for(int row = 0; row < HEIGHT; row++)
            for(int col = 0; col < WIDTH; col++) {
                // history queue separator
                result += sizeof(history[row][col].size());
                for(auto& c : history[row][col])
                    result += c.serialize();
            }
        return result;
    }
};

template<typename T>
concept DerivedTable = std::is_base_of_v<abstract_table, T>;


#endif //TABLE_H

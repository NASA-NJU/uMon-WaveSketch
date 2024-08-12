//
// Created by Thor on 2023/12/28.
//

#include "io_helper.h"
#include "benchmark.h"
#include "Wavelet/heavy.h"

STREAM parse_csv_full(const string& fname) {
    constexpr static const int scale = 65536;
    ifstream f(fname);
    if(!f.is_open()) [[unlikely]]
                exit(-1);

    STREAM result;
    string line;
    static regex line_fmt(R"(^\((TCP|UDP)\)([0-9.]+):(\d+)<>([0-9.]+):(\d+),\d*?(\d{1,9}),(\d+),\d+\s*$)");

    int i = 0;
    /*auto file_size = count_if(istreambuf_iterator<char>{f}, {},
                              [](char c) { return c == '\n'; }) + 1;
    cout << "total lines: " << file_size << endl;
    cout << string(file_size / scale, '-') << endl;*/
    while(getline(f, line)) {
        smatch match;
        if(regex_match(line, match, line_fmt)) {
            five_tuple t(match[2].str(),
                         match[4].str(),
                         match[3].str(),
                         match[5].str(),
                         match[1].str());
#ifdef SELECT_IN
            if(t.hash() % BUCKET_HEAVY == breakpoint.hash() % BUCKET_HEAVY)
#endif
            result[t].emplace_back(stoul(match[6].str()), stoi(match[7].str()));
        }
        i++;
        if(i % scale == 0)
            cout << '+' << flush;
    }
    cout << endl;

    return result;
}
SORTED parse_csv_simple(const string& fname) {
    constexpr static const int scale = 65536 * 8;
    ifstream f(fname);
    if(!f.is_open()) [[unlikely]]
                exit(-1);

    SORTED result;

    uint32_t i = 0;
    uint32_t id, len, qlen;
    uint32_t time;
    char comma;

    // ignore first line
    f.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    while(f >> id >> comma >> len >> comma >> time >> comma >> qlen) {
        five_tuple ft(id);
#ifdef SELECT_IN
        if(ft.hash() % HALF_WIDTH == breakpoint.hash() % HALF_WIDTH)
#endif
        result.emplace_back(ft, time / TIMESCALE + 1);
        i++;
        if(i % scale == 0)
            cout << '+' << flush;
#ifdef FILTER_TIME
        if(time >= FILTER_TIME) [[unlikely]]
            break;
#endif
    }

    cout << endl;
    f.close();

    sort(result.begin(), result.end(),
         [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });

    return result;
}

STREAM sum_by_flow(const SORTED& data) {
    STREAM result;
    for(auto& p : data) {
        auto& q = result[p.first];
        if(q.empty() || q.back().first < p.second)
            q.emplace_back(p.second, 1);
        else
            q.back().second++;
    }
    return result;
}

// align rhs to lhs: assume rhs only differs from lhs in DATA value
void align(STREAM_QUEUE lhs, STREAM_QUEUE& rhs) {
    transform(lhs.begin(), lhs.end(), lhs.begin(),
              [](const auto& p){ return make_pair(p.first, 0); });

    auto l_it = lhs.begin();
    auto r_it = rhs.begin();
    while(l_it != lhs.end() && r_it != rhs.end()) {
        if(l_it->first < r_it->first)
            l_it++;
        else if(l_it->first > r_it->first)
            r_it++;
        else {
            l_it->second = r_it->second;
            l_it++;
            r_it++;
        }
    }

    rhs.swap(lhs);
}
void align(const STREAM& lhs, STREAM& rhs) {
    for(auto& p : lhs) {
        auto& l_queue = p.second;
        auto& r_queue = rhs[p.first];
        align(l_queue, r_queue);
    }
}

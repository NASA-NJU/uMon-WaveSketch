#ifndef IO_HELPER_H
#define IO_HELPER_H

#include "Utility/headers.h"
#include "benchmark.h"

using namespace std;

/* csv parser */
STREAM parse_csv_full(const string& fname);
SORTED parse_csv_simple(const string& fname);
STREAM sum_by_flow(const SORTED& data);

/* deque alignment */
void align(STREAM_QUEUE lhs, STREAM_QUEUE& rhs);
void align(const STREAM& lhs, STREAM& rhs);

/* flow report */
void flow_report(const STREAM& dict, ostream& fs, const methods m);

template<DerivedScheme S>
inline void forward_transform(S& model, const SORTED& data, ostream& ms, const methods method) {
    auto start_time = chrono::high_resolution_clock::now();

    for(auto& t : data)
        model.count(get<0>(t), get<1>(t), get<2>(t));
    model.flush();

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> time_diff = end_time - start_time;
    ms << method << "," << MEMORY << "," << time_diff.count() << "," << model.serialize();
}
template<DerivedScheme S>
inline STREAM inverse_transform(S& model, const STREAM& dict, ostream& ms, const methods method) {
    auto start_time = chrono::high_resolution_clock::now();

    STREAM result = model.rebuild(dict);

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> time_diff = end_time - start_time;
    ms << "," << time_diff.count() << endl;

    return result;
}
template<DerivedScheme S>
void test(S& model, const SORTED& input, const STREAM& dict, ostream& os, ostream& fs, ostream& ms, const methods method) {
    model.reset();
    forward_transform(model, input, ms, method);
    auto result = inverse_transform(model, dict, ms, method);

    flow_report(result, fs, method);

    align(dict, result);
    compare(dict, result, os, method);
    model.reset();
}


#endif //IO_HELPER_H

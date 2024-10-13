#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "Utility/headers.h"

using namespace std;

/* methods and namespaces */
enum class methods : uint8_t {
    WAVE_IDEAL,
    WAVE_PRACTICAL,
    WAVE_ALT_I,
    WAVE_ALT_P,
    OMNIWINDOW,
    NAIVE_CMS,
    FOURIER,
    PERSIST_CMS,
    PERSIST_AMS,
    REFERENCE
};
ostream& operator<<(ostream& os, const methods& t);

class benchmark {
    methods type;
    five_tuple key;
    uint32_t recorded;
    uint32_t original;

    double l1_norm;
    double l2_norm;
    double avg_err;
    double energy;
    double cos_dis;
    double gd_l1_norm;
    double gd_l2_norm;
    double gd_energy;
    double gd_cos_dis;

public:
    constexpr static const uint32_t sketch_size = MEMORY;
    constexpr static const char format[] = "class,memory,id,length,l1,l2,are,energy,cos,g-l1,g-l2,g-energy,g-cos";
    benchmark(methods t, const five_tuple& f, const STREAM_QUEUE& lhs, const STREAM_QUEUE& rhs);
    friend ostream& operator<<(ostream& os, const benchmark& t);
};

void compare(const STREAM& lhs, const STREAM& rhs, ostream& os, const methods type);


#endif //BENCHMARK_H

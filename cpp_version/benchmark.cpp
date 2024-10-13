//
// Created by Thor on 2023/12/28.
//

#include "benchmark.h"

/* benchmarks, compare right to left */
// calculate gradient
template<typename C, typename T=C::value_type>
inline void gradient(C& vec) {
    if(vec.size() < 2) [[unlikely]]
        return;

    T last = vec[0];
    vec[0] = vec[1] - vec[0];
    for (size_t i = 1; i < vec.size() - 1; i++) {
        T temp = vec[i];
        vec[i] = (vec[i + 1] - last) / 2;
        last = temp;
    }
    vec.back() -= last;
}

// calculate L1 norm
template<typename C>
inline double L1Norm(const C& lhs, const C& rhs) {
    assert(lhs.size() == rhs.size());

    double sum = 0.;
    for (size_t i = 0; i < lhs.size(); i++) {
        sum += abs(lhs[i] - rhs[i]);
    }
    return sum;
}

// calculate L2 norm
template<typename C>
inline double L2Norm(const C& lhs, const C& rhs) {
    assert(lhs.size() == rhs.size());

    double sum = 0.;
    for (size_t i = 0; i < lhs.size(); i++) {
        sum += (lhs[i] - rhs[i]) * (lhs[i] - rhs[i]);
    }
    return sqrt(sum);
}

// calculate average error (L1 difference / original)
template<typename C>
inline double AvgError(const C& lhs, const C& rhs) {
    assert(lhs.size() == rhs.size());

    double sum = 0.;
    for (size_t i = 0; i < lhs.size(); i++) {
        auto temp = fabs(lhs[i] - rhs[i]) / (fabs(lhs[i]) + 1);
        sum += temp;
    }
    return sum / lhs.size();
}

// calculate retained energy (L2 norm ratio)
template<typename C>
inline double Energy(const C& lhs, const C& rhs) {
    assert(lhs.size() == rhs.size());

    double norm1 = 0.;
    double norm2 = 0.;

    for (size_t i = 0; i < lhs.size(); i++) {
        norm1 += lhs[i] * lhs[i];
        norm2 += rhs[i] * rhs[i];
    }

    if(norm1 == 0) {
        norm1 += 1;
        norm2 += 1;
    }
    double result = norm2 / norm1;
    if(result > 1)
        result = 1 / result;
    return result;
}

// calculate cosine distance
template<typename C>
inline double CosDistance(const C& lhs, const C& rhs) {
    assert(lhs.size() == rhs.size());

    double dotProduct = 0.;
    double norm1 = 0.;
    double norm2 = 0.;

    for (size_t i = 0; i < lhs.size(); i++) {
        dotProduct += lhs[i] * rhs[i];
        norm1 += lhs[i] * lhs[i];
        norm2 += rhs[i] * rhs[i];
    }

    if(norm1 == 0. || norm2 == 0.) [[unlikely]]
        return 0;

    double result = dotProduct / (sqrt(norm1) * sqrt(norm2));
    return result;
}

ostream& operator<<(ostream& os, const methods& t) {
    switch(t) {
        case methods::WAVE_IDEAL:
            os << "Wavelet-Ideal"; break;
        case methods::WAVE_PRACTICAL:
            os << "Wavelet-Practical"; break;
        case methods::OMNIWINDOW:
            os << "OmniWindow"; break;
        case methods::NAIVE_CMS:
            os << "Naive-Sketch"; break;
        case methods::FOURIER:
            os << "Fourier"; break;
        case methods::PERSIST_CMS:
            os << "Persist-CMS"; break;
        case methods::PERSIST_AMS:
            os << "Persist-AMS"; break;
        case methods::WAVE_ALT_I:
            os << "Wavelet-Alt-Ideal"; break;
        case methods::WAVE_ALT_P:
            os << "Wavelet-Alt-Practical"; break;
        case methods::REFERENCE:
            os << "dst" << breakpoint.dst_ip; break;
    }
    return os;
}

benchmark::benchmark(const methods t, const five_tuple &f, const STREAM_QUEUE &lhs, const STREAM_QUEUE &rhs) : type(t), key(f) {
    recorded = rhs.size();
    original = lhs.size();

    vector<double> l_vec(lhs.size());
    vector<double> r_vec(rhs.size());

    transform(lhs.begin(), lhs.end(), l_vec.begin(),
              [](const auto& p) -> double { return p.second; });
    transform(rhs.begin(), rhs.end(), r_vec.begin(),
              [](const auto& p) -> double { return p.second; });

    l1_norm = L1Norm(l_vec, r_vec);
    l2_norm = L2Norm(l_vec, r_vec);
    avg_err = AvgError(l_vec, r_vec);
    energy = Energy(l_vec, r_vec);
    cos_dis = CosDistance(l_vec, r_vec);

    gradient(l_vec);
    gradient(r_vec);

    gd_l1_norm = L1Norm(l_vec, r_vec);
    gd_l2_norm = L2Norm(l_vec, r_vec);
    gd_energy = Energy(l_vec, r_vec);
    gd_cos_dis = CosDistance(l_vec, r_vec);
}

ostream &operator<<(ostream &os, const benchmark &t) {
    os << t.type << "," << benchmark::sketch_size << "," << t.key.dst_ip << "," << t.original
       << "," << t.l1_norm
       << "," << t.l2_norm
       << "," << t.avg_err
       << "," << t.energy
       << "," << t.cos_dis
       << "," << t.gd_l1_norm
       << "," << t.gd_l2_norm
       << "," << t.gd_energy
       << "," << t.gd_cos_dis;

    return os;
}

void compare(const STREAM& lhs, const STREAM& rhs, ostream& os, const methods type) {
    const static STREAM_QUEUE default_queue;
    for(auto &o: lhs) {
#ifdef SELECT_OUT
        if(o.first.hash() % HALF_WIDTH != breakpoint.hash() % HALF_WIDTH)
            continue;
#endif
#ifdef FILTER_LOW
        if(o.second.size() < FILTER_LOW)
            continue;
#endif

        auto &l_queue = o.second;
        auto &r_queue = rhs.contains(o.first) ? rhs.find(o.first)->second : default_queue;

        benchmark p(type, o.first, l_queue, r_queue);
        os << p << endl;
    }
}

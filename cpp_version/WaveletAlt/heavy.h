#ifndef WAVELET_ALT_HEAVY_H
#define WAVELET_ALT_HEAVY_H

#include "../Utility/headers.h"
#include "../Wavelet/heavy.h"

using namespace std;

namespace WaveletAlt {

    template<unsigned QUEUE_N = 1>
    class heavy : public Wavelet::heavy<QUEUE_N> {

    };

} // WaveletAlt

#endif //WAVELET_ALT_HEAVY_H

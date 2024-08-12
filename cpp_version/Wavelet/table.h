#ifndef WAVELET_TABLE_H
#define WAVELET_TABLE_H

#include "../Utility/headers.h"
#include "counter.h"

using namespace std;

namespace Wavelet {

    template<unsigned QUEUE_N = 1>
    class table : public basic_table<counter<QUEUE_N>, FULL_WIDTH, LESS_HEIGHT> {

    };

} // Wavelet

#endif //WAVELET_TABLE_H

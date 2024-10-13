#ifndef FOURIER_TABLE_H
#define FOURIER_TABLE_H

#include "../Utility/headers.h"
#include "counter.h"

using namespace std;

namespace Fourier {

class table : public basic_table<counter, ROUND(FULL_WIDTH * (FULL_DEPTH * 4 + 4), (((FULL_DEPTH * 4) / 6) * 6 + 4 * SAMPLE_RATE * 2 + 10))> {

    };

} // Fourier

#endif //FOURIER_TABLE_H

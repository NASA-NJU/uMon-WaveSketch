#ifndef FOURIER_RECORD_H
#define FOURIER_RECORD_H

#include "../Utility/headers.h"

using namespace std;

namespace Fourier {

    struct record {
        uint16_t pos;
        float data;

        constexpr float get_data() const {
            return data;
        }

        friend constexpr partial_ordering operator<=>(const record& lhs, const record& rhs);
    };

    constexpr partial_ordering operator<=>(const record& lhs, const record& rhs) {
        return abs(lhs.data) <=> abs(rhs.data);
    }

} // Fourier

#endif //FOURIER_RECORD_H

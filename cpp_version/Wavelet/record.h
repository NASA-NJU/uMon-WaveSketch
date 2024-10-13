#ifndef WAVELET_RECORD_H
#define WAVELET_RECORD_H

#include "../Utility/headers.h"

using namespace std;

namespace Wavelet {

    struct record {
        uint16_t pos : 14;
        bool sqrt : 1;
        bool sign : 1;
        uint16_t normalized;

        record() : pos(0), sqrt(false), sign(false), normalized(0) {};
        record(uint16_t p, DATA d) {
            pos = p;
            normalized = abs(d) << ((LEVEL - 1 - level()) / 2);
            sign = d & 0x8000;
            sqrt = !(level() & 1);
        }

        constexpr uint8_t level() const {
            return countr_zero((pos | (INDEX_MASK + 1)));
        }
        constexpr DATA data() const {
            DATA data = normalized >> ((LEVEL - 1 - level()) / 2);
            return sign ? -data : data;
        }
        constexpr DATA get_data() const {
            return data();
        }

        size_t serialize() const {
            size_t result = 0;
            result += sizeof(*this);
            // *(uint32_t)(this);
            return result;
        }

        friend constexpr strong_ordering operator<=>(const record& lhs, const record& rhs);
    };

    constexpr strong_ordering operator<=>(const record& lhs, const record& rhs) {
        uint32_t l = lhs.normalized * (NOSQRT + lhs.sqrt * SQRT2B);
        uint32_t r = rhs.normalized * (NOSQRT + rhs.sqrt * SQRT2B);
        return l <=> r;
    }

} // Wavelet

#endif //WAVELET_RECORD_H

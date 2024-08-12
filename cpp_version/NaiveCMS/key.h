#ifndef NAIVE_CMS_KEY_H
#define NAIVE_CMS_KEY_H

#include "../Utility/headers.h"

using namespace std;

namespace NaiveCMS {

    struct key {
        TIME time;
        five_tuple flow;

        key(five_tuple f = {}, uint32_t t = 0) {
            flow = f;
            time = t;
        }

        size_t hash(int seed = 0) const {
            size_t hash_value = 0;
            simple_hash(this, 17, seed, &hash_value);
            return hash_value;
        }
    };

} // NaiveCMS

namespace std {
    template<>
    class hash<NaiveCMS::key> {
    public:
        size_t operator()(const NaiveCMS::key& k) const {
            return k.hash();
        }
    };
}

#endif //NAIVE_CMS_KEY_H

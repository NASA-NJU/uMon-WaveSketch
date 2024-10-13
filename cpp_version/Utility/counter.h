#ifndef COUNTER_H
#define COUNTER_H

#include "types.h"

using namespace std;

class abstract_counter {
public:
    // reset all related data structures; act as an empty counter afterward
    virtual void reset() = 0;
    // count individual packet arriving at time t, return true only if full
    virtual bool count(TIME t, HASH h, DATA c) = 0;
    // finish recording and deal with remaining buffered data
    virtual void flush() { };
    // rebuild counters in a period with timestamps
    virtual STREAM_QUEUE rebuild(HASH h) const = 0;
    // test if the counter is empty
    virtual bool empty() const = 0;
    // return timestamp of first packet, inclusive
    virtual TIME start() const = 0;
    // serialize the contents in counter
    virtual size_t serialize() const = 0;
};

template<typename C>
concept DerivedCounter = std::is_base_of_v<abstract_counter, C>;

#endif //COUNTER_H

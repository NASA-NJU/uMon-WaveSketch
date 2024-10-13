#ifndef SCHEME_H
#define SCHEME_H

#include <array>

#include "types.h"

using namespace std;

class abstract_scheme {
protected:
    template<typename T, size_t N, size_t... Ints>
    constexpr array<T, N> array_iota_impl(index_sequence<Ints...>) {
        return {Ints...};
    }
    template<typename T, size_t N>
    constexpr array<T, N> array_iota() {
        return array_iota_impl<T, N>(make_index_sequence<N>{});
    }
public:
    // reset all related data structures
    virtual void reset() = 0;
    // count individual packet arriving at time t
    virtual void count(const five_tuple& f, const TIME t, const DATA c) = 0;
    // finish recording and deal with remaining buffered data
    virtual void flush() = 0;
    // rebuild counters for a label-set in all available timestamps
    virtual STREAM rebuild(const STREAM& dict) const = 0;
    // serialize related data structures
    virtual size_t serialize() const = 0;
};

template<DerivedTable T>
class basic_scheme : public abstract_scheme {
protected:
    T sketch{};
public:
    void reset() override {
        sketch.reset();
    }

    void count(const five_tuple& f, const TIME t, const DATA c) override {
        sketch.count(f, t, c);
    }

    void flush() override {
        sketch.flush();
    }

    STREAM rebuild(const STREAM& dict) const override {
        STREAM result;
        for(auto& p : dict)
            result[p.first] = sketch.rebuild(p.first, p.second.front().first, p.second.back().first);

        return result;
    }

    virtual size_t serialize() const override {
        return sketch.serialize();
    }
};

template<typename T>
concept DerivedScheme = std::is_base_of_v<abstract_scheme, T>;


#endif //SCHEME_H

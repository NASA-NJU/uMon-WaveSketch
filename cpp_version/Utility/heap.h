#ifndef HEAP_H
#define HEAP_H

#include "parameter.h"

#include <cstdint>
#include <array>
#include <algorithm>
#include <utility>
#include <functional>
#include <cmath>
#include <cstring>
#include <random>

#define HEAP_PARENT(p) ((p - 1) / 2)
#define HEAP_LEFT(p) (2 * p + 1)

using namespace std;

// serializability concept
template<typename T>
concept Serializable = requires(T a) {
    { a.serialize() } -> std::convertible_to<std::size_t>;
};

template<Serializable T>
class abstract_heap {
public:
    virtual void reset() = 0;
    virtual T insert(T r) = 0;
    virtual const T* begin() const = 0;
    virtual const T* end() const = 0;
    virtual size_t serialize() const = 0;
};

template<Serializable T, uint32_t SIZE>
class heap : public abstract_heap<T> {
protected:
    T push(T r) {
        heap_data[size] = r;
        size++;
        push_heap(heap_data, heap_data + size, greater<>());
        return {};
    }
    T replace(T r) {
        const int last_idx = SIZE - 1;
        const int max_parent = HEAP_PARENT(last_idx);
        T old = heap_data[0];
        if(old > r)
            return r;

        int idx = 0;
        while(idx <= max_parent) {
            int child = HEAP_LEFT(idx);
            if(child < last_idx && (heap_data[child] > heap_data[child + 1]))
                child++;
            if(r < heap_data[child])
                break;
            heap_data[idx] = heap_data[child];
            idx = child;
        }
        heap_data[idx] = r;
        return old;
    }
public:
    T heap_data[SIZE]{};
    uint16_t size = 0;

    heap() = default;
    void reset() {
        size = 0;
    }

    /*template<class... Args>
    T insert(Args&&... args) {
        return insert(T(args...));
    }*/
    T insert(T r) {
        if(size < SIZE)
            return push(r);
        else
            return replace(r);
    }

    const T* begin() const {
        return heap_data;
    }
    const T* end() const {
        return heap_data + size;
    }

    template<uint32_t I>
    heap<T, SIZE>& operator=(heap<T, I> other) {
        static_assert(SIZE >= I);
        size = other.size;
        memcpy(heap_data, other.heap_data, I * sizeof(T));
        return *this;
    }

    size_t serialize() const override {
        size_t result = 0;
        result += sizeof(size);
        for(int i = 0; i < size; i++)
            result += heap_data[i].serialize();
        return result;
    }
};

template<Serializable T, uint32_t SIZE>
// a priority queue approximated by threshold-based array; when full, randomly evicts historical data
class pseudo_heap : public abstract_heap<T> {
protected:
    static mt19937 gen;

    T push_hi(T r) {
        heap_data[size_hi] = r;
        if(size_lo < size_hi)
            size_lo = size_hi;
        size_hi++;
        return {};
    }
    T push_lo(T r) {
        heap_data[size_lo] = r;
        size_lo--;
        return {};
    }
    T replace_hi(T r) {
        uniform_int_distribution<> dis(0, size_hi - 1);
        auto idx = dis(gen);
        T old = heap_data[idx];
        heap_data[idx] = r;
        return old;
    }
    T replace_lo(T r) {
        uniform_int_distribution<> dis(size_hi, SIZE - 1);
        auto idx = dis(gen);
        T old = heap_data[idx];
        heap_data[idx] = r;
        return old;
    }
public:
    T heap_data[SIZE]{};
    uint16_t size_hi = 0;
    int16_t size_lo = SIZE - 1;
    static T thresh_hi;
    static T thresh_lo;

    pseudo_heap() = default;
    void reset() {
        size_hi = 0;
        size_lo = SIZE - 1;
    }

    T insert(T r) {
        if(r < thresh_lo)
            return r;
        else if(r >= thresh_hi) [[unlikely]] {
            if(size_hi < SIZE)
                return push_hi(r);
            else [[unlikely]]
                return replace_hi(r);
        } else {
            if(size_hi <= size_lo)
                return push_lo(r);
            else if(size_hi < SIZE)
                return replace_lo(r);
            else
                return r;
        }
    }

    const T* begin() const {
        return heap_data;
    }
    const T* end() const {
        abort();
        return heap_data + size_hi;
    }

    size_t serialize() const override {
        size_t result = 0;
        result += sizeof(size_hi);
        for(int i = 0; i < size_hi; i++)
            result += heap_data[i].serialize();
        for(int i = SIZE - 1; i > size_lo; i--)
            result += heap_data[i].serialize();
        return result;
    }
};

template<Serializable T, uint32_t SIZE>
mt19937 pseudo_heap<T, SIZE>::gen(0xAEABDC85);
template<Serializable T, uint32_t SIZE>
T pseudo_heap<T, SIZE>::thresh_hi{};//740, 49};
template<Serializable T, uint32_t SIZE>
T pseudo_heap<T, SIZE>::thresh_lo{};//740, 49};


#endif //HEAP_H

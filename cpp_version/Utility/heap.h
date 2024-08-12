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

#define HEAP_PARENT(p) ((p - 1) / 2)
#define HEAP_LEFT(p) (2 * p + 1)

using namespace std;

template<typename T>
class abstract_heap {
public:
    virtual void reset() = 0;
    virtual T insert(T r) = 0;
    virtual const T* begin() const = 0;
    virtual const T* end() const = 0;
};

template<typename T, uint32_t SIZE>
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
};

using namespace std;

#endif //HEAP_H

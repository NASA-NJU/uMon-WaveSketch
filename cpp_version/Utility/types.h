#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <deque>

#include "five_tuple.h"

using namespace std;

typedef int32_t DATA;
typedef uint32_t TIME;
typedef size_t HASH;
typedef uint8_t BYTE;

typedef unordered_set<five_tuple> LABELS;
typedef deque<pair<TIME, DATA>> STREAM_QUEUE;
typedef unordered_map<five_tuple, STREAM_QUEUE> STREAM;
typedef deque<tuple<five_tuple, TIME, DATA>> SORTED;

#endif //TYPES_H

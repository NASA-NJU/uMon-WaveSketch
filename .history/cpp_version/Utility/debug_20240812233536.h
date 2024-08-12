#ifndef DEBUG_H
#define DEBUG_H

#include <cassert>

#include "five_tuple.h"

/* preprocess */
// #define SELECT_IN
// #define SELECT_OUT
// #define FILTER_LOW 20u
#define FILE_IN ("data_source/hadoop15.csv")
#define FILE_OUT ("report.csv")
// #define FILTER_TIME (65536u * TIMESCALE)

static five_tuple breakpoint(63669);

/* execution */
#define USE_WAVE_IDEAL methods::WAVE_IDEAL
#define USE_WAVE_PRACTICAL methods::WAVE_PRACTICAL
#define USE_OMNIWINDOW methods::OMNIWINDOW
#define USE_NAIVE_CMS methods::NAIVE_CMS
#define USE_FOURIER methods::FOURIER
#define USE_PERSIST_CMS methods::PERSIST_CMS
#define USE_PERSIST_AMS methods::PERSIST_AMS
// #define USE_WAVE_ALT_I methods::WAVE_ALT_I
// #define USE_WAVE_ALT_P methods::WAVE_ALT_P

#endif //DEBUG_H

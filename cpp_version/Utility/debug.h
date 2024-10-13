#ifndef DEBUG_H
#define DEBUG_H

#include <cassert>

#include "five_tuple.h"

/* preprocess */
// #define SELECT_IN
// #define SELECT_OUT
// #define FILTER_LOW 20u
#define FILE_IN ("/home/xyhan/forge/niffler/data_source/single.csv")
#define FILE_OUT ("report.csv")
#define FLOW_OUT ("sample.csv")
//#define META_OUT ("meta_report.csv")
//#define FILTER_TIME (25308u * TIMESCALE)
//#define BY_BYTES 1

static five_tuple breakpoint(2882);

/* execution */
#define USE_WAVE_IDEAL methods::WAVE_IDEAL
#define USE_WAVE_PRACTICAL methods::WAVE_PRACTICAL
#define USE_OMNIWINDOW methods::OMNIWINDOW
#define USE_NAIVE_CMS methods::NAIVE_CMS
#define USE_FOURIER methods::FOURIER
#define USE_PERSIST_CMS methods::PERSIST_CMS
#define USE_PERSIST_AMS methods::PERSIST_AMS
//#define USE_WAVE_ALT_I methods::WAVE_ALT_I
//#define USE_WAVE_ALT_P methods::WAVE_ALT_P

#endif //DEBUG_H

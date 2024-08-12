#ifndef PARAMETER_H
#define PARAMETER_H

#include <bit>

// scale for input time(ns)
#define TIMESCALE 1000u
// binary approximation of sqrt(2)
#define SQRT2F 1.4140625f
#define SQRT2B 0b00110101
#define NOSQRT 0b10000000
// one object can process data in MAX_LENGTH * TIMESCALE ns
#define MAX_LENGTH 2048u
// process data to the third top level, inclusive
#define LEVEL (countr_zero(MAX_LENGTH) - 3)
#define INDEX_MASK ((1u << LEVEL) - 1)
// maximum of data stored in heaps
#define SAMPLE_RATE 32u
#define HIST_SIZE (MAX_LENGTH / SAMPLE_RATE)
// data in second top level
#define RESERVED 8u
// table dimensions
#define FULL_WIDTH 128u
#define HALF_WIDTH (FULL_WIDTH / 2u)
#define FULL_HEIGHT 3u
#define LESS_HEIGHT (FULL_HEIGHT - 1u)
#define FULL_DEPTH (MAX_LENGTH / SAMPLE_RATE)
#define LESS_DEPTH 2u

#define BUCKET (FULL_WIDTH * FULL_HEIGHT)
#define MEMORY (FULL_WIDTH * FULL_HEIGHT * FULL_DEPTH * 4)
// window for FFT
#define WINDOW (SAMPLE_RATE * 2u)
// score multiplier for stored flow
#define HIT_RATIO 8u
#define RETAIN_THRESH (FULL_DEPTH * 4u)

#endif //PARAMETER_H
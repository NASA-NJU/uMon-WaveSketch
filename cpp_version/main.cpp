#include <iostream>
#include "Utility/headers.h"
#include "io_helper.h"
#include "benchmark.h"

#include "OmniWindow/omniwindow.h"
#include "Fourier/fourier.h"
#include "PersistCMS/persistCMS.h"
#include "PersistAMS/persistAMS.h"
#include "Wavelet/wavelet.h"
#include "NaiveCMS/naiveCMS.h"
#include "WaveletAlt/wavelet_alt.h"

using namespace std;

int main() {
    auto start_time = chrono::high_resolution_clock::now();
    auto input = parse_csv_simple(FILE_IN);
    auto parse_time = chrono::high_resolution_clock::now();
    chrono::duration<double> parse_diff = parse_time - start_time;
    cerr << "parse time: " << parse_diff.count() << "s" << endl;

    auto dict = sum_by_flow(input);

#ifdef FILE_OUT
    ofstream os(FILE_OUT, ios_base::out | ios_base::app);
    if(!os) [[unlikely]]
        exit(-1);
    if(os.tellp() == 0)
        os << benchmark::format << endl;
#else
    ostream& os = cout;
#endif
#ifdef FLOW_OUT
    ofstream fs(FLOW_OUT, ios_base::out);
    if(!fs) [[unlikely]]
        exit(-1);
    if(fs.tellp() == 0) {
        fs << "class,memory,time,data" << endl;
        flow_report(dict, fs, methods::REFERENCE);
    }
#else
    ostream& fs = cout;
#endif
#ifdef META_OUT
    ofstream ms(META_OUT, ios_base::out | ios_base::app);
    if(!ms) [[unlikely]]
        exit(-1);
    if(ms.tellp() == 0)
        ms << "class,memory,transform-time,size,rebuild-time" << endl;
#else
    ostream& ms = cerr;
#endif

#ifdef USE_NAIVE_CMS
    static naiveCMS scheme1{};
    test(scheme1, input, dict, os, fs, ms, USE_NAIVE_CMS);
#endif
#ifdef USE_OMNIWINDOW
    static omniwindow scheme2{};
    test(scheme2, input, dict, os, fs, ms, USE_OMNIWINDOW);
#endif
#ifdef USE_FOURIER
    static fourier scheme3{};
    test(scheme3, input, dict, os, fs, ms, USE_FOURIER);
#endif
#ifdef USE_PERSIST_CMS
    static persistCMS scheme4{};
    test(scheme4, input, dict, os, fs, ms, USE_PERSIST_CMS);
#endif
#ifdef USE_PERSIST_AMS
    static persistAMS scheme5{};
    test(scheme5, input, dict, os, fs, ms, USE_PERSIST_AMS);
#endif
#ifdef USE_WAVE_IDEAL
    static wavelet<false> scheme6{};
    test(scheme6, input, dict, os, fs, ms, USE_WAVE_IDEAL);
#endif
#ifdef USE_WAVE_PRACTICAL
    static wavelet<true> scheme7{};
    test(scheme7, input, dict, os, fs, ms, USE_WAVE_PRACTICAL);
#endif
#ifdef USE_WAVE_ALT_I
    static wavelet_alt<1> scheme8{};
    test(scheme8, input, dict, os, fs, ms, USE_WAVE_ALT_I);
#endif
#ifdef USE_WAVE_ALT_P
    static wavelet_alt<2> scheme9{};
    test(scheme9, input, dict, os, fs, ms, USE_WAVE_ALT_P);
#endif

    return 0;
}

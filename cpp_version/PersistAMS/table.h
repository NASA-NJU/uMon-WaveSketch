#ifndef PERSIST_AMS_TABLE_H
#define PERSIST_AMS_TABLE_H

#include "../Utility/headers.h"
#include "counter.h"

using namespace std;

namespace PersistAMS {

    class table : public basic_table<counter> {
    protected:
        DATA select_val(array<DATA, HEIGHT>& vals) const override {
            return select_median(vals);
        }
    };

} // PersistAMS

#endif //PERSIST_AMS_TABLE_H

#ifndef PERSIST_CMS_TABLE_H
#define PERSIST_CMS_TABLE_H

#include "../Utility/headers.h"
#include "counter.h"

using namespace std;

namespace PersistCMS {

    class table : public basic_table<counter> {
    protected:
        DATA select_val(array<DATA, HEIGHT>& vals) const override {
            return select_median(vals);
        }
    };

} // PersistCMS

#endif //PERSIST_CMS_TABLE_H

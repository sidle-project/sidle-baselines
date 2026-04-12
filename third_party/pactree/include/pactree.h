// SPDX-FileCopyrightText: Copyright (c) 2019-2021 Virginia Tech
// SPDX-License-Identifier: Apache-2.0

#ifndef pactreeAPI_H
#define pactreeAPI_H
#include "pac_common.h"
#include "pactreeImpl.h"

class pactree{
private:
    pactreeImpl *pt;
public:
    pactree(int numa, int cxl_percentage = 0);
    ~pactree();
    bool insert(Key_t key, Val_t val);
    bool update(Key_t key, Val_t val);
    Val_t lookup(Key_t key);
    Val_t remove(Key_t key);
    uint64_t scan(Key_t startKey, int range, std::vector<Val_t> &result);
    void registerThread();
    void unregisterThread();
};

#endif 

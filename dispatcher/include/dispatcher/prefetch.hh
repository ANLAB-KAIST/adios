#pragma once

#include <dispatcher/common.hh>

namespace dispatcher {

struct prefetch_none_t {
    int prefetch(uintptr_t fault_addr, size_t hits) {
        (void)fault_addr;
        (void)hits;
        return 0;
    }
};

struct prefetch_readahead_t {
    uintptr_t prev_fault_addr = 0;
    unsigned long prev_win = 0;
    static constexpr int max_pages = (1 << 3);
    int prefetch(uintptr_t fault_addr, size_t hits) {
        unsigned long offset = fault_addr >> 12;
        unsigned long prev_offset = prev_fault_addr >> 12;

        bool increasing = offset > prev_offset ? true : false;
        /*
         * This heuristic has been found to work well on both sequential and
         * random loads, swapping to hard disk or to SSD: please don't ask
         * what the "+ 2" means, it just happens to work well, that's all.
         */
        unsigned int pages = hits + 2;
        unsigned int last_ra = 0;

        if (pages == 2) {
            /*
             * We can have no readahead hits to judge by: but must not get
             * stuck here forever, so check for an adjacent offset instead
             * (and don't even bother to check whether swap type is same).
             */
            if (offset != prev_offset + 1 && offset != prev_offset - 1)
                pages = 1;

        } else {
            unsigned int roundup = 4;
            while (roundup < pages) roundup <<= 1;
            pages = roundup;
        }

        if (pages > max_pages) pages = max_pages;

        /* Don't shrink readahead too fast */
        last_ra = prev_win / 2;
        if (pages < last_ra) pages = last_ra;

        unsigned long win = pages - 1;

        int ret = increasing ? win : -win;

        prev_fault_addr = fault_addr;
        prev_win = win;
        return ret;
    }
};

using prefetch_t = prefetch_readahead_t;

}  // namespace dispatcher

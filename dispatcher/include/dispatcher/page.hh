#pragma once

#include <atomic>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <dispatcher/common.hh>
#include <dispatcher/spinlock.hh>
#include <protocol/net.hh>

namespace dispatcher {

namespace bi = boost::intrusive;

struct page_t {
    uintptr_t va;
    void *ptep;
    bi::list_member_hook<> hook;

    page_t() : va(0), ptep(nullptr) {}
    // this require ownership
    void reset() {
        va = 0;
        ptep = nullptr;
    }
    page_t(const page_t &) = delete;
    page_t &operator=(const page_t &) = delete;
};
using page_slice_t =
    bi::list<page_t,
             bi::member_hook<page_t, bi::list_member_hook<>, &page_t::hook>,
             bi::constant_time_size<true>>;
struct page_mgmt_t {
    int init(void *start, void *end) {
        _start = (uintptr_t)start;
        _end = (uintptr_t)end;
        num_pages = (_end - _start) >> NET_PAGE_SHIFT;
        ipt = std::vector<page_t>(num_pages);
        return 0;
    }
    // addr -> page
    inline page_t &lookup(void *ptr) {
        return ipt[(((uintptr_t)ptr) - _start) >> NET_PAGE_SHIFT];
    }

    // page -> addr
    inline void *lookup(page_t &page) {
        uintptr_t addr = _start + ((&page - ipt.data()) << NET_PAGE_SHIFT);
        return (void *)addr;
    }

    uintptr_t _start, _end;
    size_t num_pages;
    std::vector<page_t> ipt;
    page_slice_t active_list;
    spinlock_t active_lock;
    // spinlock_t clean_lock; // clean_list do not requre lock. only used by
    // reclaimer

    inline void push_pages_active(page_slice_t &pages) {
        auto page = pages.begin();

        while (page != pages.end()) {
            page++;
        }
        active_lock.lock();
        active_list.splice(active_list.end(), pages);
        active_lock.unlock();
    }

    inline void pop_pages_active_all(page_slice_t &pages) {
        active_lock.lock();
        pages.splice(pages.end(), active_list);
        active_lock.unlock();
    }
};

constexpr size_t active_flush_size = 32;
struct page_mgmt_local_t {
    page_slice_t active_buffer;
    struct page_mgmt_t *page_mgmt;

    int init(struct page_mgmt_t *page_mgmt) {
        this->page_mgmt = page_mgmt;
        return 0;
    }

    inline void try_flush_buffered() {
        if (active_buffer.size() >= active_flush_size) {
            page_mgmt->push_pages_active(active_buffer);
        }
    }

    // called by all threads
    inline int add_active_bufferd(void *paddr, uintptr_t va, void *ptep) {
        page_t &pg = page_mgmt->lookup(paddr);
        pg.va = va;
        pg.ptep = ptep;

        if (gcc_unlikely(pg.hook.is_linked())) {
            return 1;
        }
        active_buffer.push_back(pg);
        try_flush_buffered();
        return 0;
    }
};

}  // namespace dispatcher
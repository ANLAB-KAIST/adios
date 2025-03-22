#pragma once
#include <atomic>
#include <config/static-config.hh>
#include <cstdint>
#include <cstdlib>
#include <dispatcher/traces.hh>

namespace dispatcher {

template <size_t N = 511>
struct batch_t {
    union {
        uintptr_t next;
        batch_t<N> *next_bch;
        uint64_t cnt;
    };
    void *pages[N];

    batch_t() = delete;
    batch_t(const batch_t &) = delete;
    batch_t(batch_t &&) = delete;

    inline static struct batch_t *new_from_page(void *page) {
        struct batch_t *btch = (struct batch_t *)page;
        btch->cnt = 0;
        return btch;
    }

    inline void *alloc() {
        if (__builtin_expect(cnt != 0, 1)) {
            --cnt;
            return pages[cnt];
        }
        return nullptr;
    }
    inline bool free(void *page) {
        if (__builtin_expect(cnt != N, 1)) {
            pages[cnt] = page;
            ++cnt;
            return true;
        }
        return false;
    }
};

static_assert(sizeof(struct batch_t<511>) == 4096);
static_assert(sizeof(struct batch_t<1023>) == 8192);

template <size_t N = 511>
struct pagepool_global_t {
    std::atomic_uintptr_t head{0};
    std::atomic_uint64_t num_bch{0};

    inline uint64_t count_bch() {
        return num_bch.load(std::memory_order_relaxed);
    }
    inline uint64_t count_pages() { return count_bch() * (N + 1); }

    inline void free(struct batch_t<N> *btch) {
        do {
            btch->next = head.load();
        } while (!head.compare_exchange_weak(btch->next, (uintptr_t)btch));
        num_bch.fetch_add(1, std::memory_order_relaxed);
    }

    inline struct batch_t<N> *alloc_polling() {
        do {
            uintptr_t btch_u = head.load();
            if (__builtin_expect(btch_u != 0, 1)) {
                struct batch_t<N> *btch = (struct batch_t<N> *)btch_u;
                if (__builtin_expect(
                        head.compare_exchange_weak(btch_u, btch->next), true)) {
                    btch->cnt = N;
                    num_bch.fetch_sub(1, std::memory_order_relaxed);
                    return btch;
                }
            } else {
                // wait until free
                while (btch_u == 0) {
                    btch_u = head.load();
                }
            }
        } while (1);
    }

    inline struct batch_t<N> *
    alloc_non_polling() {
        do {
            uintptr_t btch_u = head.load();
            if (__builtin_expect(btch_u != 0, 1)) {
                struct batch_t<N> *btch = (struct batch_t<N> *)btch_u;
                if (__builtin_expect(
                        head.compare_exchange_weak(btch_u, btch->next), true)) {
                    btch->cnt = N;
                    num_bch.fetch_sub(1, std::memory_order_relaxed);
                    return btch;
                }
            } else {
                return nullptr;
            }
        } while (1);
    }
};

template <size_t N = 511>
struct pagepool_local_t {
    pagepool_global_t<N> *global;
    batch_t<N> *current;

    int init(pagepool_global_t<N> *global) {
        this->global = global;
        this->current = nullptr;
        return 0;
    }

    inline void *alloc_page() {
        do {
            if (__builtin_expect(current != nullptr, 1)) {
                void *page = current->alloc();
                if (__builtin_expect(page != nullptr, 1)) {
                    return page;
                }
                page = (void *)current;
                current = nullptr;
                return page;
            }
        } while (current = global->alloc_polling());

        abort();
        return nullptr;
    }

    inline void *alloc_page_non_polling() {
        do {
            if (__builtin_expect(current != nullptr, 1)) {
                void *page = current->alloc();
                if (__builtin_expect(page != nullptr, 1)) {
                    return page;
                }
                page = (void *)current;
                current = nullptr;
                return page;
            }
        } while (current = global->alloc_non_polling());

        return nullptr;
    }

    inline void free_page(void *page) {
        if (__builtin_expect(current != nullptr, 1)) {
            bool ret = current->free(page);
            if (__builtin_expect(ret, true)) {
                return;
            }
            global->free(current);
            current = nullptr;
        } else {
            current = batch_t<N>::new_from_page(page);
        }
    }
};

template <size_t N = 511>
struct pagepool_local_only_t {
    batch_t<N> *current;
    batch_t<N> *head;
    std::uint64_t num_bch;

    int init() {
        this->current = nullptr;
        this->head = 0;
        this->num_bch = 0;
        return 0;
    }

    void alloc_batch() {
        if (__builtin_expect(head != nullptr, 1)) {
            current = head;
            head = head->next_bch;
            current->cnt = N;
            --num_bch;

        } else {
            current = nullptr;
        }
    }

    void free_batch() {
        current->next_bch = head;
        head = current;
        current = nullptr;
        ++num_bch;
    }
    inline void *alloc_page() {
        if (__builtin_expect(current != nullptr, 1)) {
            void *page = current->alloc();
            if (__builtin_expect(page != nullptr, 1)) {
                DP_TRACE(trace_alloc, 0, (uintptr_t)page);
                return page;
            }
            page = (void *)current;
            alloc_batch();
            DP_TRACE(trace_alloc, 1, (uintptr_t)page);
            return page;
        } else {
#ifndef DILOS_DROP_OK
            debug_early("111\n");
            abort();
#endif
            return nullptr;
        }
    }

    inline void free_page(void *page) {
        if (__builtin_expect(current != nullptr, 1)) {
            bool ret = current->free(page);
            if (__builtin_expect(ret, true)) {
                DP_TRACE(trace_free, 0, (uintptr_t)page);
                return;
            }
            free_batch();
            DP_TRACE(trace_free, 1, (uintptr_t)page);
            current = batch_t<N>::new_from_page(page);
        } else {
            DP_TRACE(trace_free, 2, (uintptr_t)page);
            current = batch_t<N>::new_from_page(page);
        }
    }
    inline void free_page_initial(void *page) {
        if (current == nullptr) {
            current = batch_t<N>::new_from_page(page);
        } else {
            free_page(page);
        }
    }
};
}  // namespace dispatcher
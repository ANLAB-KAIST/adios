// THIS IS A DUMMY OPENMP HEADER FILE
// IT IS INTENDED TO BE USED WITH A
// DUMMY OPENMP LIBRARY TO DISABLE ALL OPENMP
// CONSTRUCTS.

#ifndef OMP_H
#define OMP_H 1

#include <stdlib.h>
#include <sys/time.h>

typedef int omp_lock_t;
typedef int omp_nest_lock_t;

#ifdef __cplusplus
extern "C" {
#endif

#define NOOP ((void)0)

inline void omp_set_num_threads(int x) {
    NOOP;
}
inline int omp_get_num_threads(void) {
    return 1;
}
inline int omp_get_max_threads(void) {
    return 1;
}
inline int omp_get_thread_num(void) {
    return 0;
}
inline int omp_get_num_procs(void) {
    return 1;
}

inline int omp_in_parallel(void) {
    return 0;
};

inline void omp_set_dynamic(int x) {
    NOOP;
}
inline int omp_get_dynamic(void) {
    return 0;
}

inline void omp_set_nested(int x) {
    NOOP;
}
inline int omp_get_nested(void) {
    return 0;
}

inline void omp_init_lock(omp_lock_t* x) {
    *x = 0;
}
inline void omp_destroy_lock(omp_lock_t* x) {
    *x = -1;
}
inline void omp_set_lock(omp_lock_t* x) {
    *x = 1;
}
inline void omp_unset_lock(omp_lock_t* x) {
    *x = 0;
}
inline int omp_test_lock(omp_lock_t* x) {
    return !(*x) ? *x = 1 : 0;
} // Sorry :(

inline void omp_init_nest_lock(omp_nest_lock_t* x) {
    NOOP;
}
inline void omp_destroy_nest_lock(omp_nest_lock_t* x) {
    NOOP;
}
inline void omp_set_nest_lock(omp_nest_lock_t* x) {
    NOOP;
}
inline void omp_unset_nest_lock(omp_nest_lock_t* x) {
    NOOP;
}
inline int omp_test_nest_lock(omp_nest_lock_t* x) {
    return 1;
}

inline double omp_get_wtime(void) {
    struct timeval* times = (struct timeval*)malloc(sizeof(struct timeval));
    gettimeofday(times, NULL);
    double retval = 1.0 * times->tv_sec + 0.000001 * times->tv_usec;
    return retval;
}
inline double omp_get_wtick(void) {
    return 1.0;
}

#ifdef __cplusplus
}
#endif

#endif /* OMP_H */

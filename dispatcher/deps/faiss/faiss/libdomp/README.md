The Dummy OpenMP Library
========================

Sometimes, you have code that relies on OpenMP function calls, but you really
just want it to run in single-threaded mode without having to link to an 
OpenMP runtime. Maybe you don't have access to an OpenMP implementation, 
or maybe the link flags are a pain in the butt, or maybe you're using some
tool that keep screwing up the linking.

Fortunately, domp (the Dummy OpenMP) fixes all of that. Just include this
version of `omp.h` in your headers, and watch as all of your programs 
magically collapse to single-threaded mode!

This works by assuming that there's only one thread ever running, and having
functions that return appropriate numbers. For example, omp_get_thread_num() 
will always return 1.

Locks nominally work. Nested locks do not. omp_get_wtime() is implemented
using sys/time.h functions instead (specifically, gettimeofday()). This
may break some of the guarantees usually provided by omp_get_wtime(), but 
seems to be accurate enough for everyday use.

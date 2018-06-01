//
// Created by Srdan Milakovic on 6/1/18.
//

#include <stdlib.h>
#include "reduction-inline.h"
#include "barrier.h"
#include "broadcast.h"

/*
 * Supported reduction operations
 */

#define AND_OP(A, B)  ((A) & (B))
#define MAX_OP(A, B)  ((A) > (B) ? (A) : (B))
#define MIN_OP(A, B)  ((A) < (B) ? (A) : (B))
#define SUM_OP(A, B)  ((A) + (B))
#define PROD_OP(A, B) ((A) * (B))
#define OR_OP(A, B)   ((A) | (B))
#define XOR_OP(A, B)  ((A) ^ (B))

/*
 * Definitions for all reductions
 */

#define SHCOLL_REDUCE_DEFINE(_name)                         \
    /* AND operation */                                     \
    _name(short_and,        short,      AND_OP)             \
    _name(int_and,          int,        AND_OP)             \
    _name(long_and,         long,       AND_OP)             \
    _name(longlong_and,     long long,  AND_OP)             \
                                                            \
    /* MAX operation */                                     \
    _name(short_max,        short,          MAX_OP)         \
    _name(int_max,          int,            MAX_OP)         \
    _name(double_max,       double,         MAX_OP)         \
    _name(float_max,        float,          MAX_OP)         \
    _name(long_max,         long,           MAX_OP)         \
    _name(longdouble_max,   long double,    MAX_OP)         \
    _name(longlong_max,     long long,      MAX_OP)         \
                                                            \
    /* MIN operation */                                     \
    _name(short_min,        short,          MIN_OP)         \
    _name(int_min,          int,            MIN_OP)         \
    _name(double_min,       double,         MIN_OP)         \
    _name(float_min,        float,          MIN_OP)         \
    _name(long_min,         long,           MIN_OP)         \
    _name(longdouble_min,   long double,    MIN_OP)         \
    _name(longlong_min,     long long,      MIN_OP)         \
                                                            \
    /* SUM operation */                                     \
    _name(complexd_sum,     double _Complex,    SUM_OP)     \
    _name(complexf_sum,     float _Complex,     SUM_OP)     \
    _name(short_sum,        short,              SUM_OP)     \
    _name(int_sum,          int,                SUM_OP)     \
    _name(double_sum,       double,             SUM_OP)     \
    _name(float_sum,        float,              SUM_OP)     \
    _name(long_sum,         long,               SUM_OP)     \
    _name(longdouble_sum,   long double,        SUM_OP)     \
    _name(longlong_sum,     long long,          SUM_OP)     \
                                                            \
    /* PROD operation */                                    \
    _name(complexd_prod,    double _Complex,    PROD_OP)    \
    _name(complexf_prod,    float _Complex,     PROD_OP)    \
    _name(short_prod,       short,              PROD_OP)    \
    _name(int_prod,         int,                PROD_OP)    \
    _name(double_prod,      double,             PROD_OP)    \
    _name(float_prod,       float,              PROD_OP)    \
    _name(long_prod,        long,               PROD_OP)    \
    _name(longdouble_prod,  long double,        PROD_OP)    \
    _name(longlong_prod,    long long,          PROD_OP)    \
                                                            \
    /* OR operation */                                      \
    _name(short_or,         short,      OR_OP)              \
    _name(int_or,           int,        OR_OP)              \
    _name(long_or,          long,       OR_OP)              \
    _name(longlong_or,      long long,  OR_OP)              \
                                                            \
    /* XOR operation */                                     \
    _name(short_xor,        short,      XOR_OP)             \
    _name(int_xor,          int,        XOR_OP)             \
    _name(long_xor,         long,       XOR_OP)             \
    _name(longlong_xor,     long long,  XOR_OP)             \


/*
 * Local reductions
 */

typedef void (*local_reduce_func)(void *, const void *, const void *, size_t);

#define REDUCE_HELPER_LOCAL(_name, _type, _op)                                                                  \
inline static void local_##_name##_reduce(void *dest, const void *src1, const void *src2, size_t nreduce) {     \
    size_t i;                                                                                                   \
    for (i = 0; i < nreduce; i++) {                                                                             \
        ((_type*) dest)[i] = _op(((_type*) src1)[i], ((_type*)src2)[i]);                                        \
    }                                                                                                           \
}                                                                                                               \


SHCOLL_REDUCE_DEFINE(REDUCE_HELPER_LOCAL)


/*
 * Linear reduction implementation
 */

inline static void reduce_helper_linear(void *dest, const void *source, int nreduce, int PE_start,
                                        int logPE_stride, int PE_size, void *pWrk, long *pSync,
                                        size_t type_size, local_reduce_func local_reduce) {
    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();
    const int me_as = (me - PE_start) / stride;
    const size_t nbytes = type_size * nreduce;

    void *tmp_dest;
    int i;

    shcoll_linear_barrier(PE_start, logPE_stride, PE_size, pSync);

    if (me_as == 0) {
        tmp_dest = malloc(nbytes);
        if (!tmp_dest) {
            /* TODO: raise error */
            exit(-1);
        }

        memcpy(tmp_dest, source, nbytes);

        for (i = 1; i < PE_size; i++) {
            shmem_getmem(dest, source, nbytes, PE_start + i * stride);
            local_reduce(tmp_dest, tmp_dest, dest, (size_t) nreduce);
        }

        memcpy(dest, tmp_dest, nbytes);
        free(tmp_dest);
    }

    shcoll_linear_barrier(PE_start, logPE_stride, PE_size, pSync);
    shcoll_broadcast8_linear(dest, dest, nbytes,
                             PE_start, PE_start, logPE_stride, PE_size, pSync + 1);
}


#define REDUCE_HELPER_LINEAR(_name, _type, _op)                                                             \
    void shcoll_##_name##_to_all_linear_i(_type *dest, const _type *source, int nreduce, int PE_start,      \
                                        int logPE_stride, int PE_size, _type *pWrk, long *pSync) {          \
        reduce_helper_linear(dest, source, nreduce, PE_start, logPE_stride, PE_size, pWrk, pSync,           \
                             sizeof(_type), local_##_name##_reduce);                                        \
    }                                                                                                       \


SHCOLL_REDUCE_DEFINE(REDUCE_HELPER_LINEAR)

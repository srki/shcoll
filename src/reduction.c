#include "reduction.h"
#include <stdlib.h>
#include <string.h>
#include "broadcast.h"


/*
 * Linear reduction implementation
 */

#define REDUCE_HELPER_LINEAR(_name, _type, _op)                                                             \
inline static void _name##_helper_linear(_type *dest, const _type *source, int nreduce, int PE_start,       \
                                         int logPE_stride, int PE_size, _type *pWrk, long *pSync) {         \
    const int stride = 1 << logPE_stride;                                                                   \
    const int me = shmem_my_pe();                                                                           \
    int me_as = (me - PE_start) / stride;                                                                   \
    size_t nbytes = sizeof(_type) * nreduce;                                                                \
    _type *tmp_dest;                                                                                        \
    int i, j;                                                                                               \
                                                                                                            \
    shmem_barrier(PE_start, logPE_stride, PE_size, pSync); /* TODO: use shcoll barrier? */                  \
                                                                                                            \
    if (me_as == 0) {                                                                                       \
        tmp_dest = malloc(nbytes);                                                                          \
        if (!tmp_dest) {                                                                                    \
            /* TODO: raise error */                                                                         \
            exit(0);                                                                                        \
        }                                                                                                   \
                                                                                                            \
        memcpy(tmp_dest, source, nbytes);                                                                   \
                                                                                                            \
        for (i = 1; i < PE_size; i++) {                                                                     \
            shmem_getmem(dest, source, nbytes, PE_start + i * stride);                                      \
            for (j = 0; j < nreduce; j++) {                                                                 \
                tmp_dest[j] = _op(tmp_dest[j], dest[j]);                                                    \
            }                                                                                               \
        }                                                                                                   \
                                                                                                            \
        memcpy(dest, tmp_dest, nbytes);                                                                     \
        free(tmp_dest);                                                                                     \
    }                                                                                                       \
                                                                                                            \
    shmem_barrier(PE_start, logPE_stride, PE_size, pSync); /* TODO: use shcoll barrier? */                  \
}                                                                                                           \
                                                                                                            \
void shcoll_##_name##_to_all_linear(_type *dest, const _type *source, int nreduce, int PE_start,            \
                                               int logPE_stride, int PE_size, _type *pWrk, long *pSync) {   \
    _name##_helper_linear(dest, source, nreduce, PE_start, logPE_stride, PE_size, pWrk, pSync);             \
    shcoll_broadcast8_linear(dest, dest, nreduce * sizeof(_type),                                           \
                             PE_start, PE_start, logPE_stride, PE_size, pSync + 1);                         \
}                                                                                                           \


/*
 * Binomial reduction implementation
 */

#define REDUCE_HELPER_BINOMIAL(_name, _type, _op)                                                           \
inline static void _name##_helper_binomial(_type *dest, const _type *source, int nreduce, int PE_start,     \
                                          int logPE_stride, int PE_size, _type *pWrk, long *pSync) {        \
    const int stride = 1 << logPE_stride;                                                                   \
    const int me = shmem_my_pe();                                                                           \
    int me_as = (me - PE_start) / stride;                                                                   \
    int target_as;                                                                                          \
    size_t nbytes = sizeof(_type) * nreduce;                                                                \
    _type *tmp_dest;                                                                                        \
    int i;                                                                                                  \
    unsigned mask = 0x1;                                                                                    \
    long old_pSync = SHMEM_SYNC_VALUE;                                                                      \
    long to_receive = 0;                                                                                    \
    long recv_mask;                                                                                         \
                                                                                                            \
    tmp_dest = malloc(nbytes);                                                                              \
    if (!tmp_dest) {                                                                                        \
        /* TODO: raise error */                                                                             \
        exit(0);                                                                                            \
    }                                                                                                       \
                                                                                                            \
    if (source != dest) {                                                                                   \
        memcpy(dest, source, nbytes);                                                                       \
    }                                                                                                       \
                                                                                                            \
    /* Stop if all messages are received or if there are no more PE on right */                             \
    for (mask = 0x1; !(me_as & mask) && ((me_as | mask) < PE_size); mask <<= 1) {                           \
        to_receive |= mask;                                                                                 \
    }                                                                                                       \
                                                                                                            \
    /* TODO: fix if SHMEM_SYNC_VALUE not 0 */                                                               \
    /* Wait until all messages are received */                                                              \
    while (to_receive != 0) {                                                                               \
        memcpy(tmp_dest, dest, nbytes);                                                                     \
        shmem_long_wait_until(pSync, SHMEM_CMP_NE, old_pSync);                                              \
        recv_mask = shmem_long_atomic_fetch(pSync, me);                                                     \
                                                                                                            \
        recv_mask &= to_receive;                                                                            \
        recv_mask ^= (recv_mask - 1) & recv_mask;                                                           \
                                                                                                            \
        /* Get array and reduce */                                                                          \
        target_as = (int) (me_as | recv_mask);                                                              \
        shmem_getmem(dest, dest, nbytes, PE_start + target_as * stride);                                    \
                                                                                                            \
        for (i = 0; i < nreduce; i++) {                                                                     \
            dest[i] += tmp_dest[i];                                                                         \
        }                                                                                                   \
                                                                                                            \
        /* Mark as received */                                                                              \
        to_receive &= ~recv_mask;                                                                           \
        old_pSync |= recv_mask;                                                                             \
    }                                                                                                       \
                                                                                                            \
    /* Notify parent */                                                                                     \
    if (me_as != 0) {                                                                                       \
        target_as = me_as & (me_as - 1);                                                                    \
        shmem_long_atomic_add(pSync, me_as ^ target_as, PE_start + target_as * stride);                     \
    }                                                                                                       \
                                                                                                            \
    *pSync = SHMEM_SYNC_VALUE;                                                                              \
    shmem_barrier(PE_start, logPE_stride, PE_size, pSync + 1); /* TODO: use shcoll barrier? */              \
}                                                                                                           \
                                                                                                            \
void shcoll_##_name##_to_all_binomial(_type *dest, const _type *source, int nreduce, int PE_start,          \
                                      int logPE_stride, int PE_size, _type *pWrk, long *pSync) {            \
    _name##_helper_binomial(dest, source, nreduce, PE_start, logPE_stride, PE_size, pWrk, pSync);           \
                                                                                                            \
    shcoll_broadcast8_binomial_tree(dest, dest, nreduce * sizeof(_type)     ,                               \
                                    PE_start, PE_start, logPE_stride, PE_size, pSync + 2);                  \
}                                                                                                           \


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


SHCOLL_REDUCE_DEFINE(REDUCE_HELPER_LINEAR)
SHCOLL_REDUCE_DEFINE(REDUCE_HELPER_BINOMIAL)


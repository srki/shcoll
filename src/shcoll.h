#ifndef _SHCOLL_H
#define _SHCOLL_H 1

#include <shmem.h>

#if SHMEM_MAJOR_VERSION == 1 && SHMEM_MINOR_VERSION == 3

#define shmem_sync_all shmem_barrier_all
#define shmem_long_atomic_inc shmem_long_inc
#define shmem_long_atomic_fetch_add shmem_long_fadd
#define shmem_long_atomic_add shmem_long_add
#define shmem_long_atomic_fetch shmem_long_fetch
#define shmem_size_atomic_set shmem_long_set
#define shmem_size_atomic_add shmem_long_fadd
#define shmem_size_wait_until shmem_long_wait_until
#define shmem_size_put shmem_long_put
#define shmem_size_p shmem_long_p

#include <string.h>

static void *shmem_calloc(size_t count, size_t size)
{
    void *mem = shmem_malloc(count * size);

    memset(mem, 0, count * size);
    return mem;
}

#endif

#ifndef CRAY_SHMEM_NUMVERSION

static void shmem_putmem_signal_nb(void *dest,
                                   const void *source,
                                   size_t nelems,
                                   uint64_t *sig_addr,
                                   uint64_t sig_value,
                                   int pe,
                                   void **transfer_handle)
{
    shmem_putmem_nbi(dest, source, nelems, pe);
    shmem_fence();
    shmem_uint64_p(sig_addr, sig_value, pe);
}

#endif

#define SHMEM_IPUT_NBI_DEFINITION(_size)                            \
    inline static void shmem_iput##_size##_nbi(void *dest,          \
                                               const void *source,  \
                                               ptrdiff_t dst,       \
                                               ptrdiff_t sst,       \
                                               size_t nelems,       \
                                               int pe)              \
    {                                                               \
        int i;                                                      \
        uint##_size##_t *dest_ptr =  (uint##_size##_t *) dest;      \
        const uint##_size##_t *source_ptr =                         \
            (const uint##_size##_t *) source;                       \
                                                                    \
        for (i = 0; i < nelems; i++) {                              \
            shmem_put##_size##_nbi(dest_ptr, source_ptr, 1, pe);    \
            dest_ptr += dst;                                        \
            source_ptr += sst;                                      \
        }                                                           \
    }                                                               \

SHMEM_IPUT_NBI_DEFINITION(32)
SHMEM_IPUT_NBI_DEFINITION(64)

#define SHCOLL_BCAST_SYNC_SIZE 2

#define SHCOLL_SYNC_VALUE 0

#define PE_SIZE_LOG (32)

/* TODO chose correct values */
#define SHCOLL_ALLTOALL_SYNC_SIZE 64
#define SHCOLL_ALLTOALLS_SYNC_SIZE SHMEM_ALLTOALLS_SYNC_SIZE
#define SHCOLL_BARRIER_SYNC_SIZE SHMEM_BARRIER_SYNC_SIZE
#define SHCOLL_COLLECT_SYNC_SIZE 68
#define SHCOLL_REDUCE_SYNC_SIZE (PE_SIZE_LOG * 2)
#define SHCOLL_REDUCE_MIN_WRKDATA_SIZE SHMEM_REDUCE_MIN_WRKDATA_SIZE

#define SHCOLL_ALLTOALL_DECLARATION(_name, _size)               \
    void shcoll_alltoall##_size##_##_name(void *dest,           \
                                          const void *source,   \
                                          size_t nelems,        \
                                          int PE_start,         \
                                          int logPE_stride,     \
                                          int PE_size,          \
                                          long *pSync);

SHCOLL_ALLTOALL_DECLARATION(shift_exchange_barrier, 32)
SHCOLL_ALLTOALL_DECLARATION(shift_exchange_barrier, 64)

SHCOLL_ALLTOALL_DECLARATION(shift_exchange_counter, 32)
SHCOLL_ALLTOALL_DECLARATION(shift_exchange_counter, 64)

SHCOLL_ALLTOALL_DECLARATION(shift_exchange_signal, 32)
SHCOLL_ALLTOALL_DECLARATION(shift_exchange_signal, 64)

SHCOLL_ALLTOALL_DECLARATION(xor_pairwise_exchange_barrier, 32)
SHCOLL_ALLTOALL_DECLARATION(xor_pairwise_exchange_barrier, 64)

SHCOLL_ALLTOALL_DECLARATION(xor_pairwise_exchange_counter, 32)
SHCOLL_ALLTOALL_DECLARATION(xor_pairwise_exchange_counter, 64)

SHCOLL_ALLTOALL_DECLARATION(xor_pairwise_exchange_signal, 32)
SHCOLL_ALLTOALL_DECLARATION(xor_pairwise_exchange_signal, 64)

SHCOLL_ALLTOALL_DECLARATION(color_pairwise_exchange_barrier, 32)
SHCOLL_ALLTOALL_DECLARATION(color_pairwise_exchange_barrier, 64)

SHCOLL_ALLTOALL_DECLARATION(color_pairwise_exchange_counter, 32)
SHCOLL_ALLTOALL_DECLARATION(color_pairwise_exchange_counter, 64)

SHCOLL_ALLTOALL_DECLARATION(color_pairwise_exchange_signal, 32)
SHCOLL_ALLTOALL_DECLARATION(color_pairwise_exchange_signal, 64)

#define SHCOLL_ALLTOALLS_DECLARATION(_name, _size)              \
    void shcoll_alltoalls##_size##_##_name(void *dest,          \
                                           const void *source,  \
                                           ptrdiff_t dst,       \
                                           ptrdiff_t sst,       \
                                           size_t nelems,       \
                                           int PE_start,        \
                                           int logPE_stride,    \
                                           int PE_size,         \
                                           long *pSync);

SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_barrier, 32)
SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_barrier, 64)

SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_counter, 32)
SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_counter, 64)

SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_barrier_nbi, 32)
SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_barrier_nbi, 64)

SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_counter_nbi, 32)
SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_counter_nbi, 64)


SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_barrier, 32)
SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_barrier, 64)

SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_counter, 32)
SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_counter, 64)

SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_barrier_nbi, 32)
SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_barrier_nbi, 64)

SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_counter_nbi, 32)
SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_counter_nbi, 64)


SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_barrier, 32)
SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_barrier, 64)

SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_counter, 32)
SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_counter, 64)

SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_barrier_nbi, 32)
SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_barrier_nbi, 64)

SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_counter_nbi, 32)
SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_counter_nbi, 64)

void shcoll_set_tree_degree(int tree_degree);
void shcoll_set_knomial_tree_radix_barrier(int tree_radix);

#define SHCOLL_BARRIER_SYNC_DECLARATION(_name)                  \
    void shcoll_barrier_##_name(int PE_start, int logPE_stride, \
                                int PE_size, long *pSync);      \
                                                                \
    void shcoll_barrier_all_##_name();                          \
                                                                \
    void shcoll_sync_##_name(int PE_start, int logPE_stride,    \
                             int PE_size, long *pSync);         \
                                                                \
    void shcoll_sync_all_##_name();

SHCOLL_BARRIER_SYNC_DECLARATION(linear)
SHCOLL_BARRIER_SYNC_DECLARATION(complete_tree)
SHCOLL_BARRIER_SYNC_DECLARATION(binomial_tree)
SHCOLL_BARRIER_SYNC_DECLARATION(knomial_tree)
SHCOLL_BARRIER_SYNC_DECLARATION(dissemination)

void shcoll_set_broadcast_tree_degree(int tree_degree);
void shcoll_set_broadcast_knomial_tree_radix_barrier(int tree_radix);

#define SHCOLL_BROADCAST_DECLARATION(_name, _size)              \
    void shcoll_broadcast##_size##_##_name(void *dest,          \
                                           const void *source,  \
                                           size_t nelems,       \
                                           int PE_root,         \
                                           int PE_start,        \
                                           int logPE_stride,    \
                                           int PE_size,         \
                                           long *pSync);

SHCOLL_BROADCAST_DECLARATION(linear, 8)
SHCOLL_BROADCAST_DECLARATION(linear, 16)
SHCOLL_BROADCAST_DECLARATION(linear, 32)
SHCOLL_BROADCAST_DECLARATION(linear, 64)

SHCOLL_BROADCAST_DECLARATION(complete_tree, 8)
SHCOLL_BROADCAST_DECLARATION(complete_tree, 16)
SHCOLL_BROADCAST_DECLARATION(complete_tree, 32)
SHCOLL_BROADCAST_DECLARATION(complete_tree, 64)

SHCOLL_BROADCAST_DECLARATION(binomial_tree, 8)
SHCOLL_BROADCAST_DECLARATION(binomial_tree, 16)
SHCOLL_BROADCAST_DECLARATION(binomial_tree, 32)
SHCOLL_BROADCAST_DECLARATION(binomial_tree, 64)

SHCOLL_BROADCAST_DECLARATION(knomial_tree, 8)
SHCOLL_BROADCAST_DECLARATION(knomial_tree, 16)
SHCOLL_BROADCAST_DECLARATION(knomial_tree, 32)
SHCOLL_BROADCAST_DECLARATION(knomial_tree, 64)

SHCOLL_BROADCAST_DECLARATION(knomial_tree_signal, 8)
SHCOLL_BROADCAST_DECLARATION(knomial_tree_signal, 16)
SHCOLL_BROADCAST_DECLARATION(knomial_tree_signal, 32)
SHCOLL_BROADCAST_DECLARATION(knomial_tree_signal, 64)

SHCOLL_BROADCAST_DECLARATION(scatter_collect, 8)
SHCOLL_BROADCAST_DECLARATION(scatter_collect, 16)
SHCOLL_BROADCAST_DECLARATION(scatter_collect, 32)
SHCOLL_BROADCAST_DECLARATION(scatter_collect, 64)

#define SHCOLL_COLLECT_DECLARATION(_name, _size)                \
    void shcoll_collect##_size##_##_name(void *dest,            \
                                         const void *source,    \
                                         size_t nelems,         \
                                         int PE_start,          \
                                         int logPE_stride,      \
                                         int PE_size,           \
                                         long *pSync);

SHCOLL_COLLECT_DECLARATION(linear, 32)
SHCOLL_COLLECT_DECLARATION(linear, 64)

SHCOLL_COLLECT_DECLARATION(all_linear, 32)
SHCOLL_COLLECT_DECLARATION(all_linear, 64)

SHCOLL_COLLECT_DECLARATION(all_linear1, 32)
SHCOLL_COLLECT_DECLARATION(all_linear1, 64)

SHCOLL_COLLECT_DECLARATION(rec_dbl, 32)
SHCOLL_COLLECT_DECLARATION(rec_dbl, 64)

SHCOLL_COLLECT_DECLARATION(rec_dbl_signal, 32)
SHCOLL_COLLECT_DECLARATION(rec_dbl_signal, 64)

SHCOLL_COLLECT_DECLARATION(ring, 32)
SHCOLL_COLLECT_DECLARATION(ring, 64)

SHCOLL_COLLECT_DECLARATION(bruck, 32)
SHCOLL_COLLECT_DECLARATION(bruck, 64)

SHCOLL_COLLECT_DECLARATION(bruck_no_rotate, 32)
SHCOLL_COLLECT_DECLARATION(bruck_no_rotate, 64)

#define SHCOLL_FCOLLECT_DECLARATION(_name, _size)               \
    void shcoll_fcollect##_size##_##_name(void *dest,           \
                                          const void *source,   \
                                          size_t nelems,        \
                                          int PE_start,         \
                                          int logPE_stride,     \
                                          int PE_size,          \
                                          long *pSync);

SHCOLL_FCOLLECT_DECLARATION(linear, 32)
SHCOLL_FCOLLECT_DECLARATION(linear, 64)

SHCOLL_FCOLLECT_DECLARATION(all_linear, 32)
SHCOLL_FCOLLECT_DECLARATION(all_linear, 64)

SHCOLL_FCOLLECT_DECLARATION(all_linear1, 32)
SHCOLL_FCOLLECT_DECLARATION(all_linear1, 64)

SHCOLL_FCOLLECT_DECLARATION(rec_dbl, 32)
SHCOLL_FCOLLECT_DECLARATION(rec_dbl, 64)

SHCOLL_FCOLLECT_DECLARATION(ring, 32)
SHCOLL_FCOLLECT_DECLARATION(ring, 64)

SHCOLL_FCOLLECT_DECLARATION(bruck, 32)
SHCOLL_FCOLLECT_DECLARATION(bruck, 64)

SHCOLL_FCOLLECT_DECLARATION(bruck_no_rotate, 32)
SHCOLL_FCOLLECT_DECLARATION(bruck_no_rotate, 64)

SHCOLL_FCOLLECT_DECLARATION(bruck_signal, 32)
SHCOLL_FCOLLECT_DECLARATION(bruck_signal, 64)

SHCOLL_FCOLLECT_DECLARATION(bruck_inplace, 32)
SHCOLL_FCOLLECT_DECLARATION(bruck_inplace, 64)

SHCOLL_FCOLLECT_DECLARATION(neighbour_exchange, 32)
SHCOLL_FCOLLECT_DECLARATION(neighbour_exchange, 64)

#define SHCOLL_BCAST_SYNC_SIZE 2

#define SHCOLL_SYNC_VALUE 0

#define PE_SIZE_LOG (32)

/* TODO chose correct values */
#define SHCOLL_ALLTOALL_SYNC_SIZE 64
#define SHCOLL_ALLTOALLS_SYNC_SIZE SHMEM_ALLTOALLS_SYNC_SIZE
#define SHCOLL_BARRIER_SYNC_SIZE SHMEM_BARRIER_SYNC_SIZE
#define SHCOLL_COLLECT_SYNC_SIZE 68
#define SHCOLL_REDUCE_SYNC_SIZE (PE_SIZE_LOG * 2)
#define SHCOLL_REDUCE_MIN_WRKDATA_SIZE SHMEM_REDUCE_MIN_WRKDATA_SIZE

#define SHCOLL_REDUCE_DECLARE(_name, _type, _algorithm)             \
    void shcoll_##_name##_to_all_##_algorithm(_type *dest,          \
                                              const _type *source,  \
                                              int nreduce,          \
                                              int PE_start,         \
                                              int logPE_stride,     \
                                              int PE_size,          \
                                              _type *pWrk,          \
                                              long *pSync);

#define SHCOLL_REDUCE_DECLARE_ALL(_algorithm)                           \
    /* AND operation */                                                 \
    SHCOLL_REDUCE_DECLARE(short_and,        short,      _algorithm)     \
        SHCOLL_REDUCE_DECLARE(int_and,          int,        _algorithm) \
        SHCOLL_REDUCE_DECLARE(long_and,         long,       _algorithm) \
        SHCOLL_REDUCE_DECLARE(longlong_and,     long long,  _algorithm) \
                                                                        \
        /* MAX operation */                                             \
        SHCOLL_REDUCE_DECLARE(short_max,        short,          _algorithm) \
        SHCOLL_REDUCE_DECLARE(int_max,          int,            _algorithm) \
        SHCOLL_REDUCE_DECLARE(double_max,       double,         _algorithm) \
        SHCOLL_REDUCE_DECLARE(float_max,        float,          _algorithm) \
        SHCOLL_REDUCE_DECLARE(long_max,         long,           _algorithm) \
        SHCOLL_REDUCE_DECLARE(longdouble_max,   long double,    _algorithm) \
        SHCOLL_REDUCE_DECLARE(longlong_max,     long long,      _algorithm) \
                                                                        \
        /* MIN operation */                                             \
        SHCOLL_REDUCE_DECLARE(short_min,        short,          _algorithm) \
        SHCOLL_REDUCE_DECLARE(int_min,          int,            _algorithm) \
        SHCOLL_REDUCE_DECLARE(double_min,       double,         _algorithm) \
        SHCOLL_REDUCE_DECLARE(float_min,        float,          _algorithm) \
        SHCOLL_REDUCE_DECLARE(long_min,         long,           _algorithm) \
        SHCOLL_REDUCE_DECLARE(longdouble_min,   long double,    _algorithm) \
        SHCOLL_REDUCE_DECLARE(longlong_min,     long long,      _algorithm) \
                                                                        \
        /* SUM operation */                                             \
        SHCOLL_REDUCE_DECLARE(complexd_sum,     double _Complex,    _algorithm) \
        SHCOLL_REDUCE_DECLARE(complexf_sum,     float _Complex,     _algorithm) \
        SHCOLL_REDUCE_DECLARE(short_sum,        short,              _algorithm) \
        SHCOLL_REDUCE_DECLARE(int_sum,          int,                _algorithm) \
        SHCOLL_REDUCE_DECLARE(double_sum,       double,             _algorithm) \
        SHCOLL_REDUCE_DECLARE(float_sum,        float,              _algorithm) \
        SHCOLL_REDUCE_DECLARE(long_sum,         long,               _algorithm) \
        SHCOLL_REDUCE_DECLARE(longdouble_sum,   long double,        _algorithm) \
        SHCOLL_REDUCE_DECLARE(longlong_sum,     long long,          _algorithm) \
                                                                        \
        /* PROD operation */                                            \
        SHCOLL_REDUCE_DECLARE(complexd_prod,    double _Complex,    _algorithm) \
        SHCOLL_REDUCE_DECLARE(complexf_prod,    float _Complex,     _algorithm) \
        SHCOLL_REDUCE_DECLARE(short_prod,       short,              _algorithm) \
        SHCOLL_REDUCE_DECLARE(int_prod,         int,                _algorithm) \
        SHCOLL_REDUCE_DECLARE(double_prod,      double,             _algorithm) \
        SHCOLL_REDUCE_DECLARE(float_prod,       float,              _algorithm) \
        SHCOLL_REDUCE_DECLARE(long_prod,        long,               _algorithm) \
        SHCOLL_REDUCE_DECLARE(longdouble_prod,  long double,        _algorithm) \
        SHCOLL_REDUCE_DECLARE(longlong_prod,    long long,          _algorithm) \
                                                                        \
        /* OR operation */                                              \
        SHCOLL_REDUCE_DECLARE(short_or,         short,      _algorithm) \
        SHCOLL_REDUCE_DECLARE(int_or,           int,        _algorithm) \
        SHCOLL_REDUCE_DECLARE(long_or,          long,       _algorithm) \
        SHCOLL_REDUCE_DECLARE(longlong_or,      long long,  _algorithm) \
                                                                        \
        /* XOR operation */                                             \
        SHCOLL_REDUCE_DECLARE(short_xor,        short,      _algorithm) \
        SHCOLL_REDUCE_DECLARE(int_xor,          int,        _algorithm) \
        SHCOLL_REDUCE_DECLARE(long_xor,         long,       _algorithm) \
        SHCOLL_REDUCE_DECLARE(longlong_xor,     long long,  _algorithm)

SHCOLL_REDUCE_DECLARE_ALL(linear)
SHCOLL_REDUCE_DECLARE_ALL(binomial)
SHCOLL_REDUCE_DECLARE_ALL(rec_dbl)
SHCOLL_REDUCE_DECLARE_ALL(rabenseifner)
SHCOLL_REDUCE_DECLARE_ALL(rabenseifner2)

#endif /* ! _SHCOLL_H */

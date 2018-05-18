#include <stdio.h>
#include <stdlib.h>
#include <shmem.h>
#include <string.h>

int main(void) {
    static long pSync[SHMEM_BCAST_SYNC_SIZE];
    for (int i = 0; i < SHMEM_BCAST_SYNC_SIZE; i++)
        pSync[i] = SHMEM_SYNC_VALUE;
    static long source[4], dest[4];

    shmem_init();
    int me = shmem_my_pe();
    int npes = shmem_n_pes();
    if (me == 0)
        for (int i = 0; i < 4; i++)
            source[i] = i;

    for (int i = 0; i < 1000; i++) {
        memset(dest, 0, sizeof(long) * 4);
        shmem_barrier_all(); //shmem_sync_all();
        shmem_broadcast64(dest, source, 4, 0, 0, 0, npes, pSync);
        for (int j = 0; j < 4; j++) {
            if (me != 0 && dest[j] != j) {
                printf("%d: %ld, %ld, %ld, %ld\n", me, dest[0], dest[1], dest[2], dest[3]);
            }
        }
    }
    shmem_finalize();
    return 0;
}

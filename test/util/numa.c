#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <hwloc.h>
#include <hwloc/bitmap.h>

int main(int argc, char **argv)
{
    hwloc_topology_t topo;
    int ret, num, weight, N;
    hwloc_obj_t obj;
    unsigned width, w;
    hwloc_bitmap_t numas[100], result;
    int n, m;
    char *tmp;
    bool overlap = false;

    if (2 != argc) {
        fprintf(stderr, "Usage: %s xml-filename\n", argv[0]);
        return -1;
    }

    if (0 != hwloc_topology_init(&topo)) {
        return -1;
    }
    if (0 != hwloc_topology_set_xml(topo, argv[1])) {
        fprintf(stderr, "SETXML FAILED\n");
        return -1;
    }
    /* since we are loading this from an external source, we have to
     * explicitly set a flag so hwloc sets things up correctly
     */
#if HWLOC_API_VERSION >= 0x20000
    ret = hwloc_topology_set_io_types_filter(topo, HWLOC_TYPE_FILTER_KEEP_IMPORTANT);
    if (0 != ret) {
        fprintf(stderr, "IOTYPES FAILED\n");
        hwloc_topology_destroy(topo);
        return ret;
    }
#endif
    if (0 != hwloc_topology_set_flags(topo, HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM)) {
        fprintf(stderr, "SETFLAGS FAILED\n");
        hwloc_topology_destroy(topo);
        return -1;
    }
    /* now load the topology */
    if (0 != hwloc_topology_load(topo)) {
        fprintf(stderr, "LOAD FAILED\n");
        hwloc_topology_destroy(topo);
        return -1;
    }

    num = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_NUMANODE);
    fprintf(stderr, "NUM NUMAS: %d\n", num);
    if (0 > num) {
        return -1;
    }
    width = (unsigned)num;

    N = 0;
    for (w = 0; w < width; w++) {
        /* get the object at this index */
        obj = hwloc_get_obj_by_type(topo, HWLOC_OBJ_NUMANODE, w);
        /* how many pu's are under it? */
        weight = hwloc_bitmap_weight(obj->cpuset);
        fprintf(stderr, "NUMA: %u NPUS: %d\n", w, weight);
        /* check for overlap with all preceding numas */
        for (m=0; m < N; m++) {
            if (hwloc_bitmap_intersects(obj->cpuset, numas[m])) {
                result = hwloc_bitmap_alloc();
                hwloc_bitmap_and(result, obj->cpuset, numas[m]);
                hwloc_bitmap_asprintf(&tmp, result);
                fprintf(stderr, "\tINTERSECTS NUMA %d: %s\n", m, tmp);
                hwloc_bitmap_free(result);
                free(tmp);
            }
        }
        /* cache this bitmap */
        numas[N] = hwloc_bitmap_alloc();
        hwloc_bitmap_copy(numas[N], obj->cpuset);
        ++N;
    }
    if (!overlap) {
        fprintf(stderr, "NO OVERLAP\n");
    }
}
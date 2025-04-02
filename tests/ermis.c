#define ASTRON_HELIOS_IMPLEMENTATION
#include "../helios.h"
#include "../ermis.h"

ERMIS_DECL_ARRAY(S32, IntArray);
ERMIS_IMPL_ARRAY(S32, IntArray);

int main() {
    UZ ints_count = 10;

    HeliosAllocator malloc_allocator = HeliosNewMallocAllocator();

    IntArray ints;
    IntArrayInit(&ints, malloc_allocator, ints_count);

    for (UZ i = 0; i < ints_count; ++i) {
        IntArrayPush(&ints, i);
    }

    HELIOS_VERIFY(ints.capacity == ints_count);
    HELIOS_VERIFY(ints.count == ints.capacity);

    for (UZ i = 0; i < ints_count; ++i) {
        HELIOS_VERIFY(IntArrayAt(&ints, i) == i);
    }

    return 0;
}

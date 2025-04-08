#define ASTRON_HELIOS_IMPLEMENTATION
#include "../helios.h"
#include "../ermis.h"

ERMIS_DECL_ARRAY(S32, IntArray)
ERMIS_IMPL_ARRAY(S32, IntArray)

ERMIS_DECL_HASHMAP(U32, U32, IntsMap)
ERMIS_IMPL_HASHMAP(U32, U32, IntsMap, ErmisEqFuncU32, ErmisHashFuncU32)

void test_array(void) {
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
        HELIOS_VERIFY(IntArrayAt(&ints, i) == (S32)i);
    }
}

void test_hashmap(void) {
    HeliosAllocator malloc_allocator = HeliosNewMallocAllocator();
    IntsMap map;
    IntsMapInit(&map, malloc_allocator, 500);

    HELIOS_VERIFY(map.count == 0);
    HELIOS_VERIFY(map.capacity == 500);

    for (U32 i = 0; i < 350; ++i) {
        HELIOS_VERIFY(IntsMapInsert(&map, i, i * 2));
    }

    HELIOS_VERIFY(map.capacity == 500);
    HELIOS_VERIFY(map.count == 350);

    HELIOS_VERIFY(IntsMapInsert(&map, 1000, 2000));

    HELIOS_VERIFY(map.capacity == ERMIS_HASHMAP_GROW_FACTOR(500));
    HELIOS_VERIFY(map.count == 351);
}

int main(void) {
    test_array();
    test_hashmap();
    return 0;
}

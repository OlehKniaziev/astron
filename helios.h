#ifndef ASTRON_HELIOS_H
#define ASTRON_HELIOS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>

#ifndef HELIOS_STRIP_ASSERTS
#    define HELIOS_ASSERT(cond) do {                                        \
            if (!(cond)) {                                                  \
                fprintf(stderr, "%s:%d: ASSERTION FAILED: %s", __FILE__, __LINE__, #cond); \
                abort();                                                    \
            }                                                               \
        } while (0)

#else
#    define HELIOS_ASSERT(cond)
#endif // HELIOS_STRIP_ASSERTS

#define HELIOS_VERIFY(cond) do {                                        \
        if (!(cond)) {                                                  \
            fprintf(stderr, "%s:%d: VERIFICATION FAILED: %s", __FILE__, __LINE__, #cond); \
            abort();                                                    \
        }                                                               \
    } while (0)

#define HELIOS_UNUSED(x) (void)(x)

#define HELIOS_TODO() do {                                      \
        fprintf(stderr, "%s:%d: TODO", __FILE__, __LINE__);     \
        abort();                                                \
    } while (0)

#ifdef _MSC_VER
#    define HELIOS_INLINE __forceinline
#else
#    define HELIOS_INLINE __attribute__((always_inline)) inline
#endif // compiler check

#if SIZE_MAX == UINT32_MAX
#    define HELIOS_BITS_32
#elif SIZE_MAX == UINT64_MAX
#    define HELIOS_BITS_64
#else
#    error "Could not determine architecture bit size"
#endif // word size check

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t  S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

typedef S8  B8;
typedef S16 B16;
typedef S32 B32;
typedef S64 B64;

#if defined(HELIOS_BITS_32)
    typedef uint32_t UZ;
    typedef int32_t  SZ;
#elif defined(HELIOS_BITS_64)
    typedef uint64_t UZ;
    typedef int64_t  SZ;
#endif // bit size check

typedef struct HeliosAllocatorVTable {
    void *(*alloc)(void*, UZ);        // required
    void  (*free)(void*, void*, UZ);    // required
    void *(*realloc)(void*, void*, UZ, UZ); // optional
} HeliosAllocatorVTable;

typedef struct HeliosAllocator {
    HeliosAllocatorVTable vtable;
    void *data;
} HeliosAllocator;

HELIOS_INLINE void *HeliosAlloc(HeliosAllocator allocator, UZ size) {
    return allocator.vtable.alloc(allocator.data, size);
}

HELIOS_INLINE void HeliosFree(HeliosAllocator allocator, void *ptr, UZ size) {
    allocator.vtable.free(allocator.data, ptr, size);
}

HELIOS_INLINE void *HeliosRealloc(HeliosAllocator allocator, void *old_ptr, UZ old_size, UZ size) {
    if (allocator.vtable.realloc != NULL)
        return allocator.vtable.realloc(allocator.data, old_ptr, old_size, size);

    void *new_ptr = HeliosAlloc(allocator, size);
    memcpy(new_ptr, old_ptr, old_size);
    HeliosFree(allocator, old_ptr, old_size);
    return new_ptr;
}

HeliosAllocator HeliosNewMallocAllocator(void);

#ifdef ASTRON_HELIOS_IMPLEMENTATION

    void *MallocStub(void *user, UZ size) {
        HELIOS_UNUSED(user);
        return malloc(size);
    }

    void FreeStub(void *user, void *ptr, UZ size) {
        HELIOS_UNUSED(user);
        HELIOS_UNUSED(size);
        free(ptr);
    }

    void *ReallocStub(void *user, void *ptr, UZ old_size, UZ new_size) {
        HELIOS_UNUSED(user);
        HELIOS_UNUSED(old_size);
        return realloc(ptr, new_size);
    }

    HeliosAllocator HeliosNewMallocAllocator(void) {
        return (HeliosAllocator) {
            .vtable = (HeliosAllocatorVTable) {
                .alloc = MallocStub,
                .free = FreeStub,
                .realloc = ReallocStub,
            },
            .data = NULL,
        };
    }

#endif // HELIOS_IMPLEMENTATION
#endif // HELIOS_H

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

#define HELIOS_INTERNAL static

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

typedef float  F32;
typedef double F64;

#if defined(HELIOS_BITS_32)
    typedef uint32_t UZ;
    typedef int32_t  SZ;
#elif defined(HELIOS_BITS_64)
    typedef uint64_t UZ;
    typedef int64_t  SZ;
#endif // bit size check

typedef struct HeliosAllocatorVTable {
    void *(*alloc)(void*, UZ);              // required
    void  (*free)(void*, void*, UZ);        // required
    void *(*realloc)(void*, void*, UZ, UZ); // optional
} HeliosAllocatorVTable;

typedef struct HeliosAllocator {
    HeliosAllocatorVTable vtable;
    void *data;
} HeliosAllocator;

typedef struct HeliosString8 {
    U8 *data;
    UZ count;
    UZ capacity;
    HeliosAllocator allocator;
} HeliosString8;

typedef struct HeliosStringView {
    const U8 *data;
    UZ count;
} HeliosStringView;

typedef U32 HeliosChar;

HELIOS_INLINE B32 HeliosCharIsDigit(HeliosChar c) {
    return c >= '0' && c <= '9';
}

HELIOS_INLINE B32 HeliosCharIsAlpha(HeliosChar c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

HELIOS_INLINE B32 HeliosCharIsAlnum(HeliosChar c) {
    return HeliosCharIsDigit(c) || HeliosCharIsAlpha(c);
}

HeliosString8 HeliosString8FromCStr(HeliosAllocator, const char *);
B32 HeliosString8FromCStrChecked(HeliosAllocator, const char *, HeliosString8 *);

typedef struct HeliosString8Stream {
    const U8 *data;
    UZ count;
    UZ byte_offset;
    UZ char_offset;
} HeliosString8Stream;

void HeliosString8StreamInit(HeliosString8Stream *, const U8 *, UZ);
B32 HeliosString8StreamCur(HeliosString8Stream *, HeliosChar *);
B32 HeliosString8StreamNext(HeliosString8Stream *, HeliosChar *);
void HeliosString8StreamPrev(HeliosString8Stream *, HeliosChar *);

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

void HeliosString8StreamInit(HeliosString8Stream *stream, const U8 *data, UZ count) {
    stream->data = data;
    stream->byte_offset = (UZ)-1;
    stream->char_offset = (UZ)-1;
    stream->count = count;
}

#define HELIOS_UTF8_MASK2 ((HeliosChar)0xC0 << 24)
#define HELIOS_UTF8_MASK3 ((HeliosChar)0xE0 << 24)
#define HELIOS_UTF8_MASK4 ((HeliosChar)0xF0 << 24)

// TODO: add more validations
B32 HeliosString8StreamNext(HeliosString8Stream *stream, HeliosChar *c) {
    if (stream->byte_offset >= stream->count) return 0;

    ++stream->char_offset;

    U32 bytes = *(const U32 *)(stream->data + stream->byte_offset);

    if ((bytes & HELIOS_UTF8_MASK2) == HELIOS_UTF8_MASK2) {
        if (c != NULL) *c = ((bytes & (0x0F << 24)) >> 18) | ((bytes & (0x3F << 16)) >> 16);
        stream->byte_offset += 2;
        return 1;
    }

    if ((bytes & HELIOS_UTF8_MASK3) == HELIOS_UTF8_MASK3) {
        if (c != NULL) *c = ((bytes & (0x0F << 24)) >> 12) | ((bytes & (0x3F << 16)) >> 10) | ((bytes & (0x3F << 8)) >> 8);
        stream->byte_offset += 3;
        return 1;
    }

    if ((bytes & HELIOS_UTF8_MASK4) == HELIOS_UTF8_MASK4) {
        if (c != NULL) *c = ((bytes & (0x07 << 24)) >> 6) | ((bytes & (0x3F << 16)) >> 4) | ((bytes & (0x3F << 8)) >> 2) | (bytes & 0x3F);
        stream->byte_offset += 4;
        return 1;
    }

    if (c != NULL) *c = bytes >> 24;
    ++stream->byte_offset;
    return 1;
}

#endif // HELIOS_IMPLEMENTATION
#endif // HELIOS_H

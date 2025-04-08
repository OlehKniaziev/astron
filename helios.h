#ifndef ASTRON_HELIOS_H
#define ASTRON_HELIOS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>

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

#ifdef _WIN32
#define HELIOS_PAGE_ALIGNMENT (1024 * 64)
#else
#define HELIOS_PAGE_ALIGNMENT (1024 * 4)
#endif // _WIN32

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

void *HeliosRawAlloc(UZ);

HELIOS_INLINE UZ HeliosRoundUp(UZ size, UZ align) {
    return size + ((align - (size & (align - 1))) & (align - 1));
}

HELIOS_INLINE UZ HeliosRoundDown(UZ size, UZ align) {
    return size - (size & (align - 1));
}

extern HeliosAllocator helios_temp_allocator;

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

B32 HeliosParseS64(HeliosStringView, S64 *);
B32 HeliosParseF64(HeliosStringView, F64 *);

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

HELIOS_INLINE HeliosString8 HeliosString8FromStringView(HeliosAllocator allocator, HeliosStringView sv) {
    UZ s_count = sv.count;

    U8 *s_data = (U8 *)HeliosAlloc(allocator, s_count);
    memcpy(s_data, sv.data, s_count);
    s_data[s_count] = '\0';

    return (HeliosString8) {
        .data = s_data,
        .count = s_count,
        .capacity = s_count,
        .allocator = allocator,
    };
}

#ifdef ASTRON_HELIOS_IMPLEMENTATION

B32 HeliosParseS64(HeliosStringView source, S64 *out) {
    if (source.count == 0) return 0;

    S64 result = 0;
    UZ coef = 1;

    for (UZ i = source.count - 1; i > 0; --i) {
        U8 c = source.data[i];
        if (!isdigit(c)) return 0;

        U8 digit = c - '0';
        result += digit * coef;
        coef *= 10;
    }

    if (isdigit(source.data[0])) result += (source.data[0] - '0') * coef;
    else if (source.data[0] == '-') result = -result;
    else return 0;

    *out = result;

    return 0;
}

// FIXME: This should NOT allocate anything, even on the temp allocator.
// Skill issue.
B32 HeliosParseF64(HeliosStringView source, F64 *out) {
    UZ temp_count = source.count + 1;
    char *temp = (char *)HeliosAlloc(helios_temp_allocator, temp_count);
    memcpy(temp, (const void *)source.data, source.count);
    temp[temp_count] = '\0';

    char *d_out;
    *out = strtod(temp, &d_out);
    return temp != d_out;
}

HELIOS_INTERNAL void *MallocStub(void *user, UZ size) {
    HELIOS_UNUSED(user);
    return malloc(size);
}

HELIOS_INTERNAL void FreeStub(void *user, void *ptr, UZ size) {
    HELIOS_UNUSED(user);
    HELIOS_UNUSED(size);
    free(ptr);
}

HELIOS_INTERNAL void *ReallocStub(void *user, void *ptr, UZ old_size, UZ new_size) {
    HELIOS_UNUSED(user);
    HELIOS_UNUSED(old_size);
    return realloc(ptr, new_size);
}

HELIOS_INTERNAL void _HeliosNopFreeStub(void *user, void *ptr, UZ size) {
    HELIOS_UNUSED(user);
    HELIOS_UNUSED(ptr);
    HELIOS_UNUSED(size);
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

typedef struct HeliosDynamicCircleBufferAllocator {
    void *buffer;
    UZ capacity;
    UZ offset;
} HeliosDynamicCircleBufferAllocator;

HELIOS_INTERNAL void *_HeliosDynamicCircleBufferAllocatorAlloc(void *a_ptr, UZ size) {
    HeliosDynamicCircleBufferAllocator *allocator = (HeliosDynamicCircleBufferAllocator *)a_ptr;
    if (allocator->buffer == NULL) {
        HELIOS_ASSERT(allocator->capacity % 2 == 0);
        allocator->buffer = HeliosRawAlloc(allocator->capacity);
    }

    size = HeliosRoundUp(size, sizeof(UZ));
    if (allocator->offset + size >= allocator->capacity) allocator->offset = 0;

    void *ptr = (void *)((U8 *)allocator->buffer + allocator->offset);
    allocator->offset += size;
    return ptr;
}

HeliosAllocator HeliosNewDynamicCircleBufferAllocator(HeliosDynamicCircleBufferAllocator *allocator, UZ capacity) {
    capacity = HeliosRoundUp(capacity, HELIOS_PAGE_ALIGNMENT);

    allocator->buffer = NULL;
    allocator->capacity = capacity;
    allocator->offset = 0;

    return (HeliosAllocator) {
        .data = (void *)allocator,
        .vtable = (HeliosAllocatorVTable) {
            .alloc = _HeliosDynamicCircleBufferAllocatorAlloc,
            .free = _HeliosNopFreeStub,
            .realloc = NULL,
        },
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

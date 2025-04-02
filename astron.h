#ifndef ASTRON_H
#define ASTRON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>

#ifndef ASTRON_STRIP_ASSERTS
#define ASTRON_ASSERT(cond) do {					\
		if (!(cond)) {						\
			fprintf(stderr, "%s:%d: ASSERTION FAILED: %s", __FILE__, __LINE__, #cond); \
			abort();					\
		}							\
	} while (0)

#else
#define ASTRON_ASSERT(cond)
#endif // ASTRON_STRIP_ASSERTS

#define ASTRON_VERIFY(cond) do {					\
		if (!(cond)) {						\
			fprintf(stderr, "%s:%d: VERIFICATION FAILED: %s", __FILE__, __LINE__, #cond); \
			abort();					\
		}							\
	} while (0)

#define ASTRON_UNUSED(x) (void)(x)

#ifdef _MSC_VER
#	define ASTRON_INLINE __forceinline
#else
#	define ASTRON_INLINE __attribute__((always_inline)) inline
#endif // compiler check

#if SIZE_MAX == UINT32_MAX
#  define ASTRON_BITS_32
#elif SIZE_MAX == UINT64_MAX
#  define ASTRON_BITS_64
#else
#  error "Could not determine architecture bit size"
#endif // word size check

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t  S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

#if defined(ASTRON_BITS_32)
	typedef uint32_t UZ;
	typedef int32_t  SZ;
#elif defined(ASTRON_BITS_64)
	typedef uint64_t UZ;
	typedef int64_t  SZ;
#endif // bit size check

typedef struct AstronAllocatorVTable {
	void *(*alloc)(void*, UZ);		// required
	void  (*free)(void*, void*, UZ);	// required
	void *(*realloc)(void*, void*, UZ, UZ); // optional
} AstronAllocatorVTable;

typedef struct AstronAllocator {
	AstronAllocatorVTable vtable;
	void *data;
} AstronAllocator;

ASTRON_INLINE void *AstronAlloc(AstronAllocator allocator, UZ size) {
	return allocator.vtable.alloc(allocator.data, size);
}

ASTRON_INLINE void AstronFree(AstronAllocator allocator, void *ptr, UZ size) {
	allocator.vtable.free(allocator.data, ptr, size);
}

ASTRON_INLINE void *AstronRealloc(AstronAllocator allocator, void *old_ptr, UZ old_size, UZ size) {
	if (allocator.vtable.realloc != NULL)
		return allocator.vtable.realloc(allocator.data, old_ptr, old_size, size);

	void *new_ptr = AstronAlloc(allocator, size);
	memcpy(new_ptr, old_ptr, old_size);
	AstronFree(allocator, old_ptr, old_size);
	return new_ptr;
}

AstronAllocator AstronNewMallocAllocator(void);

#ifdef ASTRON_IMPLEMENTATION

void *MallocStub(void *user, UZ size) {
	ASTRON_UNUSED(user);
	return malloc(size);
}

void FreeStub(void *user, void *ptr, UZ size) {
	ASTRON_UNUSED(user);
	ASTRON_UNUSED(size);
	free(ptr);
}

void *ReallocStub(void *user, void *ptr, UZ old_size, UZ new_size) {
	ASTRON_UNUSED(user);
	ASTRON_UNUSED(old_size);
	return realloc(ptr, new_size);
}

AstronAllocator AstronNewMallocAllocator(void) {
	return (AstronAllocator) {
		.vtable = (AstronAllocatorVTable) {
			.alloc = MallocStub,
			.free = FreeStub,
			.realloc = ReallocStub,
		},
		.data = NULL,
	};
}

#endif // ASTRON_IMPLEMENTATION
#endif // ASTRON_H

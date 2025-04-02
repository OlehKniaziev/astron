#ifndef ASTRON_PHOBOS_H
#define ASTRON_PHOBOS_H

#ifndef ASTRON_H
#    error "phobos.h requires 'astron.h' to be included first"
#endif // ASTRON_H

#define PHOBOS_ARRAY_GROW_FACTOR(x) ((((x) + 1) * 3) >> 1)

#define PHOBOS_GEN_ARRAY(T, arrname) typedef struct arrname {		\
		AstronAllocator allocator;				\
		T *items;						\
		UZ count;						\
		UZ capacity;						\
	} arrname;							\
									\
	void arrname##Init(arrname *arr, AstronAllocator allocator, UZ cap) { \
		arr->allocator = allocator;				\
		arr->capacity = cap;					\
		arr->items = AstronAlloc(allocator, sizeof(T) * cap);	\
		arr->count = 0;						\
	}								\
									\
	void arrname##Push(arrname *arr, T item) {			\
		if (arr->count >= arr->capacity) {			\
			UZ new_capacity = PHOBOS_ARRAY_GROW_FACTOR(arr->capacity); \
			arr->items = AstronRealloc(arr->allocator, arr->items, sizeof(T) * arr->capacity, sizeof(T) * new_capacity); \
			arr->capacity = new_capacity;			\
		}							\
									\
		arr->items[arr->count++] = item;			\
	}								\
									\
	T arrname##Pop(arrname *arr) {				\
		ASTRON_VERIFY(arr->count != 0);				\
		return arr->items[--arr->count];			\
	}								\
									\
	T arrname##At(arrname *arr, UZ idx) {				\
		ASTRON_VERIFY(idx < arr->count);			\
		return arr->items[idx];					\
	}								\
									\
	T *arrname##AtP(arrname *arr, UZ idx) {			\
		ASTRON_VERIFY(idx < arr->count);			\
		return &arr->items[idx];				\
	}								\
									\
	const T *arrname##AtPC(const arrname *arr, UZ idx) {		\
		ASTRON_VERIFY(idx < arr->count);			\
		return &arr->items[idx];				\
	}								\
									\
	ASTRON_INLINE void arrname##Free(arrname *arr) {		\
		AstronFree(arr->allocator, arr->items, sizeof(T) * arr->capacity); \
						       }

#endif // ASTRON_PHOBOS_H

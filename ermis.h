#ifndef ASTRON_ERMIS_H
#define ASTRON_ERMIS_H

#ifndef ASTRON_HELIOS_H
#    error "ermis.h requires 'helios.h' to be included first"
#endif // ASTRON_HELIOS_H

#define ERMIS_ARRAY_GROW_FACTOR(x) ((((x) + 1) * 3) >> 1)

#define ERMIS_DECL_ARRAY(T, arrname) typedef struct arrname {           \
        HeliosAllocator allocator;                                      \
        T *items;                                                       \
        UZ count;                                                       \
        UZ capacity;                                                    \
    } arrname;

#define ERMIS_IMPL_ARRAY(T, arrname)                                    \
    void arrname##Init(arrname *arr, HeliosAllocator allocator, UZ cap) { \
        arr->allocator = allocator;                                     \
        arr->capacity = cap;                                            \
        arr->items = HeliosAlloc(allocator, sizeof(T) * cap);           \
        arr->count = 0;                                                 \
    }                                                                   \
                                                                        \
    void arrname##Push(arrname *arr, T item) {                          \
        if (arr->count >= arr->capacity) {                              \
            UZ new_capacity = ERMIS_ARRAY_GROW_FACTOR(arr->capacity);  \
            arr->items = HeliosRealloc(arr->allocator, arr->items, sizeof(T) * arr->capacity, sizeof(T) * new_capacity); \
            arr->capacity = new_capacity;                               \
        }                                                               \
                                                                        \
        arr->items[arr->count++] = item;                                \
    }                                                                   \
                                                                        \
    T arrname##Pop(arrname *arr) {                                      \
        HELIOS_VERIFY(arr->count != 0);                                 \
        return arr->items[--arr->count];                                \
    }                                                                   \
                                                                        \
    T arrname##At(arrname *arr, UZ idx) {                               \
        HELIOS_VERIFY(idx < arr->count);                                \
        return arr->items[idx];                                         \
    }                                                                   \
                                                                        \
    T *arrname##AtP(arrname *arr, UZ idx) {                             \
        HELIOS_VERIFY(idx < arr->count);                                \
        return &arr->items[idx];                                        \
    }                                                                   \
                                                                        \
    const T *arrname##AtPC(const arrname *arr, UZ idx) {                \
        HELIOS_VERIFY(idx < arr->count);                                \
        return &arr->items[idx];                                        \
    }                                                                   \
                                                                        \
    HELIOS_INLINE void arrname##Free(arrname *arr) {                    \
        HeliosFree(arr->allocator, arr->items, sizeof(T) * arr->capacity); \
    }

#endif // ASTRON_ERMIS_H

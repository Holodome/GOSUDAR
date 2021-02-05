#if !defined(DYNAMIC_ARRAY_HH)

#include "mem.hh"

template <typename T> 
struct Array {
    T *data;
    size_t size;
    size_t capacity;
    
    bool allow_growing;
    
    Array(u32 reserve = 10, bool allow_growing = true) {
        init(reserve, allow_growing);
    }
    ~Array() {
        Mem::free(data);
    }
    
    void init(u32 reserve = 10, bool allow_growing = true) {
        size = 0;
        capacity = reserve;
        data = (T *)Mem::alloc(sizeof(T) * reserve);
        this->allow_growing = allow_growing;
    }
    
    // Adds object and returns pointer to it in array
    T *add(T o) {
        assert(capacity);
        if (size + 1 > capacity) {
            if (!allow_growing) {
                fprintf(stderr, "[ERROR] fixed array size overflow\n");
                assert(false);
            }
            void *new_data = Mem::alloc(sizeof(T) * capacity * 2);
            memcpy(new_data, data, size * sizeof(T));
            Mem::free(data);
            data = (T *)new_data;
            capacity *= 2;
        }
        data[size] = o;
        return &data[size++];
    }
    
    void clear() {
        size = 0;
    }
    
    void reserve(u32 count) {
        void *new_data = Mem::alloc(sizeof(T) * count);
        memcpy(new_data, data, count * sizeof(T));
        Mem::free(data);
        data = (T *)new_data;
        capacity = count;
    }
    
    T &operator[](size_t idx) {
        assert(idx < size);
        return data[idx];
    }
};
#define DYNAMIC_ARRAY_HH 1
#endif

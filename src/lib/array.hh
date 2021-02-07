#if !defined(DYNAMIC_ARRAY_HH)

#include "mem.hh"

template <typename T>
void *array_new(size_t capacity) {
    T *ptr = (T *)Mem::alloc_clear(capacity * sizeof(T));
    for (u32 i = 0; i < capacity; ++i) {
        ptr[i] = T();
    }
    return ptr;
}

template <typename T>
void array_delete(void *ptr, size_t capacity) {
    for (u32 i = 0; i < capacity; ++i) {
        ((T *)ptr)[i].~T();
    }
}

template <typename T>
void *array_resize(void *old_ptrv, size_t old_capacity, size_t new_capacity) {
    T *old_ptr = (T *)old_ptrv;
    T *new_ptr = 0;
    if (new_capacity) {
        new_ptr = (T *)array_new<T>(new_capacity);
        size_t overlap = old_capacity;
        if (new_capacity < old_capacity) {
            overlap = new_capacity;
        }
        for (size_t i = 0; i < overlap; ++i) {
            new_ptr[i] = old_ptr[i];
        }
    }
    array_delete<T>(old_ptr, old_capacity);
    return new_ptr;
}

template <typename T> 
struct Array {
    T *data;
    size_t size, capacity;
    
    Array() {
		clear();
    }
    ~Array() {
        array_delete<T>(data, capacity);
    }
    
    void clear() {
        if (data) {
            array_delete<T>(data, capacity);
        }
        data = 0;
        capacity = 0;
        size = 0;
    }
    
    void resize(size_t count) {
        if (!count) {
            clear();
            return;
        }    
        if (count == capacity) {
            return;
        }
        data = (T *)array_resize<T>(data, capacity, count);
        capacity = count;
        if (capacity < size) {
            size = capacity;
        }
    }
    
    size_t add(const T &obj) {
        if (size + 1 > capacity) {
            // @TODO currently we double the size of array but later we may want to increment it instead
            size_t new_capacity = capacity * 2;
            if (new_capacity == 0) {
                new_capacity = 16;
            }
            resize(new_capacity);
        }
        
        data[size] = obj;
        return size++;
    }
    
    const T &operator[](size_t idx) const {
        assert(idx < size);
        return data[idx];
    }
    
    T &operator[](size_t idx) {
        assert(idx < size);
        return data[idx];
    }
};
#define DYNAMIC_ARRAY_HH 1
#endif

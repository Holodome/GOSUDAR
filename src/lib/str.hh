#if !defined(STR_HH)

#include "lib/general.hh"
#include "lib/mem.hh"

const static size_t STR_BASE_ALLOC = 20;

struct Str {
    
    char *data;
    u32 size;
    u32 capacity;
    // @TODO see if this is useful
    char base_buffer[STR_BASE_ALLOC];
    
    Str() {
        init();
    }
    Str(char *text) {
        init();
        u32 len = strlen(text);
        ensure_alloced(len + 1);
        strcpy(data, text);
        size = len;
    } 
    ~Str() {
        if (data != base_buffer) {
            Mem::free(data);
        }
    }
    
    // Helper functions
    void init() {
        size = 0;
        capacity = STR_BASE_ALLOC;
        data = base_buffer;
        data[0] = 0;
    }
        
    void ensure_alloced(u32 amount, bool copy_old = true) {
        if (amount > capacity) {
            reallocate(amount, copy_old);
        }
    }
    
    void reallocate(u32 amount, bool copy_old) {
        assert(amount);
        
        u32 new_capacity = amount;
        char *new_data = (char *)Mem::alloc(amount);
        
        if (copy_old) {
            memcpy(new_data, data, size);
        }
        
        if (data != base_buffer) {
            Mem::free(data);
        }
        data = new_data;
    }
    
    char operator[](size_t idx) const {
        assert(size > idx);
        return data[idx];
    }
    char &operator[](size_t idx) {
        assert(size > idx);
        return data[idx];
    }
    
    bool cmp(Str &other) {
        return size == other.size && (strncmp(data, other.data, size) == 0);
    }
    
    bool cmp(char *other) {
        size_t ol = strlen(other);
        return ol == size && strncmp(data, other, size);
    }
    
    // Library funcions placed here  
    static bool cmp(const char *a, const char *b) {
        return strcmp(a, b);
    }
    
    static bool cmpn(const char *a, const char *b, size_t n) {
        return strncmp(a, b, n);
    }
    
    static void formatv(char *dst, size_t dst_size, char *format, va_list args) {
        ::vsnprintf(dst, dst_size, format, args);
    }
    
    static void format(char *dst, size_t dst_size, char *format, ...) {
        va_list args;
        va_start(args, format);
        ::vsnprintf(dst, dst_size, format, args);
        va_end(args);
    }
    
};

#define STR_HH 1
#endif

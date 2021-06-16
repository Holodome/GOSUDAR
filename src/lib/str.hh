#if !defined(STR_HH)

#include "lib/general.hh"
#include "lib/mem.hh"

const static size_t STR_BASE_ALLOC = 20;

struct Str {
    // 8
    char *data;
    // 4
    u32 len, capacity;
    // 20
    // @TODO see if this is useful
    char base_buffer[STR_BASE_ALLOC];
    
    Str() {
        init();
    }
    Str(const char *text) {
        init();
        u32 len = strlen(text);
        ensure_alloced(len + 1);
        memcpy(data, text, len);
        this->data[len] = 0;
        this->len = len;
    } 
    Str(const char *text, size_t len) {
        init();
        ensure_alloced(len + 1);
        memcpy(data, text, len);
        this->data[len] = 0;
        this->len = len;
    }
    ~Str() {
        if (data != base_buffer) {
            Mem::free(data);
        }
    }
    
    // Helper functions
    void init() {
        len = 0;
        capacity = STR_BASE_ALLOC;
        data = base_buffer;
        data[0] = 0;
    }
    
    // Make sure string has enough storage for amount chars
    void ensure_alloced(u32 amount, bool copy_old = true) {
        if (amount > capacity) {
            reallocate(amount, copy_old);
        }
    }
    
    // Change string data storage, either allocate new buffer or use base_buffer
    void reallocate(u32 amount, bool copy_old = true) {
        assert(amount);
        
        u32 new_capacity = amount;
        char *new_data;
        if (new_capacity > STR_BASE_ALLOC) {
            new_data = (char *)Mem::alloc(amount);
            capacity = new_capacity;
        } else {
            new_data = base_buffer;
            capacity = STR_BASE_ALLOC;
        }
        
        if (copy_old) {
            memcpy(new_data, data, len);
        }
        
        if (data != base_buffer) {
            Mem::free(data);
        }
        data = new_data;
    }
    
    char operator[](size_t idx) const {
        assert(len > idx);
        return data[idx];
    }
    char &operator[](size_t idx) {
        assert(len > idx);
        return data[idx];
    }
    
    bool cmp(const Str &other) const {
        return len == other.len && (strncmp(data, other.data, len) == 0);
    }
    
    bool cmp(const char *other) const {
        size_t ol = strlen(other);
        return ol == len && (strncmp(data, other, len) == 0);
    }
    
    void operator=(const Str &other) {
        size_t l = other.len;
        ensure_alloced(l + 1, false);
        memcpy(data, other.data, l);
        data[l] = 0;
        len = l;
    }
    
    void operator=(const char *text) {
        size_t l = strlen(text);
        ensure_alloced(l + 1, false);
        memcpy(data, text, l);
        data[l] = 0;
        len = l;
    }
    
    // Library funcions placed here  
    static bool cmp(const char *a, const char *b) {
        return strcmp(a, b);
    }
    
    static bool cmpn(const char *a, const char *b, size_t n) {
        return strncmp(a, b, n);
    }
    
    static size_t formatv(char *dst, size_t dst_size, const char *format, va_list args) {
        return ::vsnprintf(dst, dst_size, format, args);
    }
    
    static size_t format(char *dst, size_t dst_size, const char *format, ...) {
        va_list args;
        va_start(args, format);
        size_t len = ::vsnprintf(dst, dst_size, format, args);
        va_end(args);
        return len;
    }
    
};

#define STR_HH 1
#endif

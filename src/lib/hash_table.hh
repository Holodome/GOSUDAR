#if !defined(HASH_TABLE_HH)

#include "lib/general.hh"
#include "lib/math.hh"
#include "lib/mem.hh"
#include "lib/hashing.hh"

// Not resizable hash table
// Pretty slow, but can be used for complex data types
template <typename T>
struct HashTable {
    struct HashNode {
        Str key;
        T value;
        HashNode *next;
        
        HashNode(const char *key, const T &value, HashNode *next) {
            this->key = Str(key);
            this->value = value;
            this->next = next;
        }
    };
    int table_size;
    int num_entries;
    int table_size_mask;
    HashNode **heads;
    
    int get_hash(const char *key) const {
        return crc32(key, strlen(key)) & this->table_size_mask;
    } 
    
    HashTable(int size = 256) {
        assert(Math::is_power_of_two(size));
        this->table_size = size;
        assert(size);
        this->heads = new HashNode *[this->table_size];
        memset(this->heads, 0, sizeof(*this->heads) * this->table_size);
        this->num_entries = 0;
        this->table_size_mask = this->table_size - 1;
    }
     
    ~HashTable() {
        clear();
        delete[] heads;
    }
    
    void clear() {
        for (size_t i = 0; i < this->table_size; ++i) {
            HashNode *next = this->heads[i];
            while (next) {
                HashNode *node = next;
                next = next->next;
                delete node;
            }
            heads[i] = 0;
        }
        this->num_entries = 0;
    }
    
    void set(const char *key, const T &value) {
        int hash = get_hash(key);
        HashNode **next_ptr, *node;
        for (next_ptr = heads + hash, node = *next_ptr; node; next_ptr = &node->next, node = *next_ptr) {
            if (node->key.cmp(key)) {
                node->value = value;
                return;
            } else {
                break;
            }
        }    
        
        ++num_entries;
        *next_ptr = new HashNode(key, value, this->heads[hash]);
        (*next_ptr)->next = node;
    }
    
    bool get(const char *key, T **value = 0) const {
        HashNode *node;
        int hash = get_hash(key);
        for (node = this->heads[hash]; node; node = node->next) {
            if (node->key.cmp(key)) {
                if (value) {
                    *value = &node->value;
                }
                return true;
            }
        }
        
        if (value) {
            *value = 0;
        }
        return false;
    }
    
    bool del(const char *key) {
        int hash = get_hash(key);
        HashNode **head = this->heads + hash;
        if (*head) {
            HashNode *prev, *node;
            for (prev = 0, node = *head; node; prev = node, node = node->next) {
                if (node->key.cmp(key)) {
                    if (prev) {
                        prev->next = node->next;
                    } else {
                        *head = node->next;
                    }
                    
                    delete node;
                    --num_entries;
                    return true;
                }
            }
        }
        return false;
    }
};  

#define HASH_TABLE_HH 1
#endif

#if !defined(ENTITY_HH)

#include "lib/lib.hh"

enum struct EntityKind {
    
};  

struct Entity {
    EntityKind kind;
    
    void update();  
};

#define ENTITY_HH 1
#endif

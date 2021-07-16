//
// Order system defines how in-game orders are accessed
// There are several things that order system must be capable of:
// 1. Create new orders from description without duplicates
// (this may be expanded on single order operating on single entity
//  if we have for example order to fix the building and to destroy it
//  they are technically different, but still can't exist together)
// 2. Assign orders, unassign them and disband
// 3. Having list of pending orders that can be assigned
//
// We have to think how we want to structure more complex orders
// Maybe order system should have list of assigned entity ids 
// Of what if several orders operating on same entity can coexist
// maybe let the game handle validating order, and so when it needs to assign new one
// it will cycle through the list and remove the invalid (how does it now something is 
// invalid though ?)
#if !defined(ORDERS_HH)

#include "lib.hh"

enum {
    ORDER_NONE = 0x0,
    ORDER_CHOP,
};

// Actual order description 
struct Order {
    u32 kind;
    EntityID destination_id;
};

struct OrderListEntry {
    OrderID id;
    OrderListEntry *next;
    OrderListEntry *prev;
};

enum {
    ORDER_STATE_NONE = 0x0,
    ORDER_STATE_PENDING,
    ORDER_STATE_ASSIGNED
};

struct OrderSlot {
    OrderID id;
    u32 state;
    u32 description_hash;
    Order order;
    // Needed for deletion, we can have it stack allocated since slot exist only while list entry exists
    OrderListEntry list_entry;
    // Needed for free list
    OrderSlot *next;
};

struct OrderHash {
    OrderID id;
    OrderSlot *ptr;
};

#define ORDERS_HASH_SIZE 512
CT_ASSERT(IS_POW2(ORDERS_HASH_SIZE));

struct OrderSystem {
    // Needed to allocte orders and order list
    MemoryArena *arena;
    // Continiously incremented
    u32 last_order_id_value;
    // Size of hash is not a probles since pointers are used here
    OrderHash order_hash[ORDERS_HASH_SIZE];
    OrderListEntry order_list;
    
    u32 orders_allocated;
    OrderSlot *first_free_slot;
};

void init_order_system(OrderSystem *order_system, MemoryArena *arena);

OrderID try_to_add_order(OrderSystem *sys, Order order);
Order *get_order_by_id(OrderSystem *sys, OrderID id);

OrderID get_pending_order_id(OrderSystem *sys);
void set_order_assigned(OrderSystem *sys, OrderID id);
void set_order_unassigned(OrderSystem *sys, OrderID id);
void disband_order(OrderSystem *sys, OrderID id);

#define ORDERS_HH 1
#endif 
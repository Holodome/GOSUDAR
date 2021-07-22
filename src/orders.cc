#include "orders.hh"

u32 hash_order_description(Order order) {
    return crc32(&order, sizeof(order));
}

void init_order_system(OrderSystem *order_system, MemoryArena *arena) {
    order_system->arena = arena;
    CDLIST_INIT(&order_system->order_list);
}

OrderHash *get_order_hash(OrderSystem *sys, OrderID id) {
    if (IS_NULL(id)) {
        return 0;
    }
    
    OrderHash *result = 0;
    u32 hash_value = id.value;
    u32 hash_mask = ARRAY_SIZE(sys->order_hash) - 1;
    for (size_t offset = 0; offset < ARRAY_SIZE(sys->order_hash); ++offset) {
        u32 hash_idx = ((hash_value + offset) & hash_mask);
        OrderHash *test = sys->order_hash + hash_idx;
        if (IS_NULL(test->id) || IS_SAME(test->id, id)) {
            result = test;
            break;
        }
    }
    
    return result;
}

OrderSlot *get_order_slot_by_id(OrderSystem *sys, OrderID id) {
    if (IS_NULL(id)) {
        return 0;
    }
    
    OrderSlot *result = 0;
    OrderHash *hash = get_order_hash(sys, id);
    if (hash) {
        result = hash->ptr;
    }
    return result;
}

static OrderSlot *create_order_slot(OrderSystem *sys) {
    OrderID id = { ++sys->last_order_id_value };
    
    OrderSlot *order = sys->first_free_slot;
    if (!order) {
        ++sys->orders_allocated;
        order = alloc_struct(sys->arena, OrderSlot);
    } else {
        LLIST_POP(sys->first_free_slot);
    }
    order->id = id;
    // Add to list
    OrderListEntry *list_entry = &order->list_entry;
    list_entry->id = id;
    CDLIST_ADD(&sys->order_list, list_entry);
    
    // Add to hash
    OrderHash *hash = get_order_hash(sys, id);
    assert(hash->ptr == 0);
    hash->id = id;
    hash->ptr = order;
    return order;
}

Order *get_order_by_id(OrderSystem *sys, OrderID id) {
    Order *result = 0;
    OrderSlot *slot = get_order_slot_by_id(sys, id);
    if (slot) {
        result = &slot->order;
    }
    return result;
}

OrderID get_pending_order_id(OrderSystem *sys) {
    OrderID result = {};
    CDLIST_ITER(iter, &sys->order_list) {
        OrderID id = iter->id;
        OrderSlot *slot = get_order_slot_by_id(sys, id);
        if (slot->state == ORDER_STATE_PENDING) {
            result = id;
            break;
        }
    }
    return result;
}

OrderID try_to_add_order(OrderSystem *sys, Order order) {
    OrderID result = {};
    
    bool same_description_exists = false;
    u32 description_hash = hash_order_description(order);
    CDLIST_ITER(iter, &sys->order_list) {
        OrderID id = iter->id;
        OrderSlot *slot = get_order_slot_by_id(sys, id);
        assert(slot);
        if (slot->description_hash == description_hash) {
            same_description_exists = true;
            break;
        }
    }
    
    if (!same_description_exists) {
        OrderSlot *slot = create_order_slot(sys);
        slot->description_hash = description_hash;
        slot->order = order;
        slot->state = ORDER_STATE_PENDING;
        result = slot->id;
    }
    return result;
}

void set_order_assigned(OrderSystem *sys, OrderID id) {
    OrderSlot *slot = get_order_slot_by_id(sys, id);
    assert(slot);
    assert(slot->state == ORDER_STATE_PENDING);
    slot->state = ORDER_STATE_ASSIGNED;
}

void set_order_unassigned(OrderSystem *sys, OrderID id) {
    OrderSlot *slot = get_order_slot_by_id(sys, id);
    assert(slot);
    assert(slot->state == ORDER_STATE_ASSIGNED);
    slot->state = ORDER_STATE_PENDING;
}

void disband_order(OrderSystem *sys, OrderID id) {
    OrderHash *hash = get_order_hash(sys, id);
    OrderSlot *slot = hash->ptr;
    CDLIST_REMOVE(&slot->list_entry);
    hash->ptr = 0;
    hash->id = {};
    LLIST_ADD(sys->first_free_slot, slot);
}
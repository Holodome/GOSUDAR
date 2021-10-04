#include "platform/window.h"

void 
update_key_state(Window_State *input, u32 key, bool new_down) {
    assert(key < KEY_COUNT);
    input->keys_transition_count[key] += TO_BOOL(input->is_keys_down[key] != new_down);
    input->is_keys_down[key] = new_down;
}
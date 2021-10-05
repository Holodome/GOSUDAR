#include "filesystem.h"
#include "lib/hashing.h"
#include "lib/strings.h"

#define FS_HASH_SIZE 128

typedef struct FS_File_Slot {
    u64 hash;
    char *name; // @TODO Filepath should be here
    bool is_open;
    u32 file_mode;
    uptr file_size_cached;
    OS_File_Handle handle;
} FS_File_Slot;

typedef struct FS_Filepath_Slot {
    u64 hash;
    char *rel_path_cached;
    char *abs_path_cached;
} FSFilepathSlot;

typedef struct FS_Ctx {
    Hash64 file_hash;
    u64 nfile_slots;
    u64 nfile_slots_used;
    FS_File_Slot *file_hash_slots;
    // @TODO Free list for ids
    
    Hash64 filepath_hash;
    u64 nfilepath_slots;
    u64 nfilepath_slots_used;
    FSFilepathSlot *filepath_hash_slots;
} FS_Ctx;

static FS_Ctx *fs;

struct FS_Ctx *
create_filesystem(void) {
    struct FS_Ctx *ctx_local = mem_alloc(sizeof(FS_Ctx));
    init_filesystem(ctx_local);
    fs->nfile_slots = FS_HASH_SIZE;
    fs->file_hash = create_hash64(fs->nfile_slots);
    fs->file_hash_slots = mem_alloc_arr(fs->nfile_slots, FS_File_Slot);
    fs->nfilepath_slots = FS_HASH_SIZE;
    fs->filepath_hash = create_hash64(fs->nfilepath_slots);
    fs->filepath_hash_slots = mem_alloc_arr(fs->nfilepath_slots, FSFilepathSlot);
    fs->nfile_slots_used++;
    fs->nfilepath_slots_used++;
    return ctx_local;
}

void 
init_filesystem(struct FS_Ctx *ctx) {
    fs = ctx;
}

bool 
is_file_id_valid(File_ID id) {
    return id.value != 0;
}

static FS_File_Slot *
get_slot(u64 hash) {
    FS_File_Slot *slot = 0;
    u64 default_value = (u64)-1;
    u64 idx = hash64_get(&fs->file_hash, hash, default_value);
    if (idx != default_value) {
        assert(idx < FS_HASH_SIZE);
        slot = fs->file_hash_slots + idx;
    }
    return slot;
}

static u64 
get_hash_for_filename(const char *filename) {
    // @TOOD Use filepaths
    return hash_string(filename);
}

static u64 
get_new_file_slot_idx(void) {
    assert(fs->nfile_slots_used < fs->nfile_slots);
    return fs->nfile_slots_used++;
}

static void 
open_slot_file(FS_File_Slot *slot) {
    if (!slot->is_open) {
        os_open_file(&slot->handle, slot->name, slot->file_mode);
        slot->is_open = true;
    }
}

File_ID 
fs_get_id_for_filename(const char *filename) {
    File_ID result = {};
    // @TODO Construct filepath
    u64 filename_hash = get_hash_for_filename(filename);
    FS_File_Slot *slot = get_slot(filename_hash);
    if (slot && slot->hash) {
        result.value = slot->hash;
    }
    return result;
}

OS_File_Handle *
fs_get_handle(File_ID id) {
    OS_File_Handle *handle = 0;
    // @TODO Construct filepath
    FS_File_Slot *slot = get_slot(id.value);
    if (slot && slot->hash) {
        handle = &slot->handle;
    }
    return handle;
}

File_ID 
fs_open_file(const char *name, u32 mode) {
    File_ID result = {};
    // Check if it already open
    u64 hash = get_hash_for_filename(name);
    FS_File_Slot *slot = get_slot(hash);
    if (slot) {
        // @TODO(hl): Error reporting
        // report_error_general("File is already open: '%s'", name);
    } else {
        u64 new_slot_idx = get_new_file_slot_idx();
        if (new_slot_idx != (u64)-1) {
            hash64_set(&fs->file_hash, hash, new_slot_idx);
            slot = fs->file_hash_slots + new_slot_idx;
            slot->hash = hash;
            // @LEAK
            slot->name = mem_alloc_str(name);
            slot->file_mode = mode;
            open_slot_file(slot);
            slot->file_size_cached = (u64)-1;
            result.value = hash;
        }
    }
    
    return result;    
}

bool 
fs_close_file(File_ID id) {
    bool result = false;
    FS_File_Slot *slot = get_slot(id.value);
    if (!slot) {
        // char bf[1024] = {};
        // fs_fmt_filename(bf, sizeof(bf), id);
        // @TODO(hl): Error reporting
        // report_error_general("No file open for file id %llu (%s)", id.value, bf);
    } else {
        os_close_file(&slot->handle);
        slot->is_open = false;
        hash64_set(&fs->file_hash, slot->hash, -1);
        slot->hash = 0;
        // @TODO Think about policy for closed files - do we want to have some of their contents
        // cached, shoul we free the slot or leave it for information...
    }
    return result;
}

uptr 
fs_get_file_size(File_ID id) {
    uptr result = 0;
    FS_File_Slot *slot = get_slot(id.value);
    if (slot) {
        if (slot->file_size_cached == (u64)-1) {
            slot->file_size_cached = os_get_file_size(&slot->handle);
        }
        result = slot->file_size_cached;
    }
    return result;    
}

uptr 
fs_fmt_filename(char *bf, uptr bf_sz, File_ID id) {
    uptr result = 0;
    FS_File_Slot *slot = get_slot(id.value);
    if (slot) {
        result = str_cp(bf, bf_sz, slot->name);
    }
    return result;    
}

void 
DBG_dump_file(const char *filename, const void *data, u64 data_size) {
    OS_File_Handle handle = {};
    os_open_file(&handle, filename, FILE_MODE_WRITE);
    os_write_file(&handle, 0, data, data_size);
    os_close_file(&handle);    
}
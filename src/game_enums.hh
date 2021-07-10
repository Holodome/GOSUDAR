#if !defined(ENTITY_KINDS_HH)

#include "general.hh"

struct EnumString {
    const char *string;
    u32 value;  
};
#define ENUM_STRING_(_string, _value) {_string, _value}
#define ENUM_STRING(_value) ENUM_STRING_(#_value, _value)

#define DEFINE_ENUM_(_enum_name, _enum_values) enum _enum_name { _enum_values };
#define DEFINE_ENUM(_enum_name, _enum_values) DEFINE_ENUM_(_enum_name, _enum_values)
#define DEFINE_ENUM_STRINGS_(_enum_name, _enum_values) const EnumString _enum_name##_strings[] = { _enum_values };
#define DEFINE_ENUM_STRINGS(_enum_name, _enum_values) DEFINE_ENUM_STRINGS_(_enum_name, _enum_values)

// Lookup serialized enum value
// This is better of using some hash table, but since this code is executed very rarely we
// probably shouldn't care
#define DEBUG_enum_string_lookup(_strings, _test, _dst) DEBUG_enum_string_lookup_(_strings, ARRAY_SIZE(_strings), _test, _dst)
inline bool DEBUG_enum_string_lookup_(const EnumString *strings, size_t strings_count, const char *test, u32 *dst) {
    bool found = false;
    for (size_t i = 0; i < strings_count; ++i) {
        const EnumString *test_id = strings + i;
        if (strcmp(test_id->string, test) == 0) {
            found = true;
            *dst = test_id->value;
            break;
        }
    }
    return found;
}

//
// Entity kinds
// So the reason all different level entity enums are placed in one and have values assigned to them
// is that we want the same values for different entity types, and since asset system uses this 
// we want to avoid confusion, where some integer value is assigned to different entity kind
// This is more verbose to assign values manually, but helps to avoid countless bugs
// What we want to do later is have some checksum for all serialized enums - so we can check asset files 
// if they have the same checksum and eassily tell that they are from the same version of the game
// If maintaining this verbose structure can become hard - we may switch to having some tool that generate enum tables for us
// Or later we can remove the notion of entity enum completely - let text files define enum values,
// so there can be arbitrary number of enttiy kinds, each of which can have its own set of actually in-game defined behavoiurs
//
#define ENTITY_KIND_ENUM_NAME EntityKind
#define ENTITY_KINDS                                    \
ENTITY_KIND(ENTITY_KIND_NONE,         0x0)              \
ENTITY_KIND(ENTITY_KIND_PLAYER,       0x1)              \
ENTITY_KIND(ENTITY_KIND_WORLD_OBJECT, 0x2)              \
ENTITY_KIND(ENTITY_KIND_RESERVED3,    0x3)              \
ENTITY_KIND(ENTITY_KIND_RESERVED4,    0x4)              \
ENTITY_KIND(ENTITY_KIND_RESERVED5,    0x5)              \
ENTITY_KIND(ENTITY_KIND_RESERVED6,    0x6)              \
ENTITY_KIND(ENTITY_KIND_RESERVED7,    0x7)              \
ENTITY_KIND(ENTITY_KIND_RESERVED8,    0x10)             \
ENTITY_KIND(ENTITY_KIND_RESERVED9,    0x11)             \
ENTITY_KIND(ENTITY_KIND_RESERVED10,   0x12)             \
\
ENTITY_KIND(WORLD_OBJECT_KIND_NONE,          0x0)       \
ENTITY_KIND(WORLD_OBJECT_KIND_TREE_FOREST,   0x1)       \
ENTITY_KIND(WORLD_OBJECT_KIND_TREE_DESERT,   0x2)       \
ENTITY_KIND(WORLD_OBJECT_KIND_TREE_JUNGLE,   0x3)       \
ENTITY_KIND(WORLD_OBJECT_KIND_GOLD_DEPOSIT,  0x4)       \
ENTITY_KIND(WORLD_OBJECT_KIND_BUILDING1,     0x5)       \
ENTITY_KIND(WORLD_OBJECT_KIND_BUILDING2,     0x6)       \
ENTITY_KIND(WORLD_OBJECT_KIND_RESERVED7,     0x7)       \
ENTITY_KIND(WORLD_OBJECT_KIND_RESERVED8,     0x10)      \
ENTITY_KIND(WORLD_OBJECT_KIND_RESERVED9,     0x11)      \
ENTITY_KIND(WORLD_OBJECT_KIND_RESERVED10,    0x12)      \
\
ENTITY_KIND(RESOURCE_KIND_NONE, 0x0)                    \
ENTITY_KIND(RESOURCE_KIND_WOOD, 0x1)                    \
ENTITY_KIND(RESOURCE_KIND_GOLD, 0x2)                    \
ENTITY_KIND(RESOURCE_KIND_RESERVED3, 0x3)               \
ENTITY_KIND(RESOURCE_KIND_RESERVED4, 0x4)               \

#define ENTITY_KIND(_name, _value) _name = _value,
DEFINE_ENUM(ENTITY_KIND_ENUM_NAME, ENTITY_KINDS)
#undef ENTITY_KIND
#define ENTITY_KIND(_name, _value) ENUM_STRING_(#_name, _value),
DEFINE_ENUM_STRINGS(ENTITY_KIND_ENUM_NAME, ENTITY_KINDS)
#undef ENTITY_KIND

#undef ENTITY_KINDS
#undef ENTITY_KIND_ENUM_NAME

//
// Asset tags
//

#define ASSET_TAGS_ENUM_NAME AssetTagID
#define ASSET_TAGS                          \
ASSET_TAG(ASSET_TAG_BIOME, 0)               \
ASSET_TAG(ASSET_TAG_WORLD_OBJECT_KIND, 0)   \
ASSET_TAG(ASSET_TAG_BUILDING_IS_BUILT, 1)

#define ASSET_TAG(_name, _value) _name = _value,
DEFINE_ENUM(ASSET_TAGS_ENUM_NAME, ASSET_TAGS)
#undef ASSET_TAG
#define ASSET_TAG(_name, _value) ENUM_STRING_(#_name, _value),
DEFINE_ENUM_STRINGS(ASSET_TAGS_ENUM_NAME, ASSET_TAGS)
#undef ASSET_TAG
#undef ASSET_TAGS
#undef ASSET_TAGS_ENUM_NAME

//
// Asset types
//

#define ASSET_TYPES_ENUM_NAME AssetType
#define ASSET_TYPES                    \
ASSET_TYPE(ASSET_TYPE_NONE)            \
ASSET_TYPE(ASSET_TYPE_WORLD_OBJECT)    \
ASSET_TYPE(ASSET_TYPE_PLAYER)          \
ASSET_TYPE(ASSET_TYPE_FONT)            \
ASSET_TYPE(ASSET_TYPE_GRASS)           \
ASSET_TYPE(ASSET_TYPE_ADDITIONAL)      \
ASSET_TYPE(ASSET_TYPE_SOUND)           \
ASSET_TYPE(ASSET_TYPE_SENTINEL)        \

#define ASSET_TYPE(_name) _name,
DEFINE_ENUM(ASSET_TYPES_ENUM_NAME, ASSET_TYPES)
#undef ASSET_TYPE
#define ASSET_TYPE(_name) ENUM_STRING(_name),
DEFINE_ENUM_STRINGS(ASSET_TYPES_ENUM_NAME, ASSET_TYPES)
#undef ASSET_TYPE
#undef ASSET_TYPES
#undef ASSET_TYPES_ENUM_NAME

//
// Flags
// No need to serialize them
//

#define ENTITY_KINDS_HH 1
#endif

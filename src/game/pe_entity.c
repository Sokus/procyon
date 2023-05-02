#include "pe_entity.h"
#include "pe_core.h"
#include "pe_config.h"

#include "pe_bit_stream.h"

#include "HandmadeMath.h"

#include <stdint.h>

peEntity *entities = NULL;
uint16_t *free_indices = NULL;
int free_indices_count = 0;

peSerializationError pe_serialize_vec2(peBitStream *bs, HMM_Vec2 *value) {
    peSerializationError err = peSerializationError_None;
    err = pe_serialize_float(bs, &value->X); if (err) return err;
    err = pe_serialize_float(bs, &value->Y); if (err) return err;
    return err;
}

peSerializationError pe_serialize_vec3(peBitStream *bs, HMM_Vec3 *value) {
    peSerializationError err = peSerializationError_None;
    err = pe_serialize_float(bs, &value->X); if (err) return err;
    err = pe_serialize_float(bs, &value->Y); if (err) return err;
    err = pe_serialize_float(bs, &value->Z); if (err) return err;
    return err;
}

peSerializationError pe_serialize_input(peBitStream *bs, peInput *input) {
    peSerializationError err = peSerializationError_None;
    err = pe_serialize_vec2(bs, &input->movement); if (err) return err;
    err = pe_serialize_float(bs, &input->angle); if (err) return err;
    return err;
}

peSerializationError pe_serialize_entity(peBitStream *bs, peEntity *entity) {
    peSerializationError err = peSerializationError_None;
    err = pe_serialize_u32(bs, &entity->index); if (err) return err;
    err = pe_serialize_bool(bs, &entity->active); if (err) return err;
    if (!entity->active) return err;

    err = pe_serialize_bits(bs, &entity->properties, peEntityProperty_Count); if (err) return err;
    err = pe_serialize_vec3(bs, &entity->position); if (err) return err;
    err = pe_serialize_vec3(bs, &entity->velocity); if (err) return err;
    err = pe_serialize_float(bs, &entity->angle); if (err) return err;

    if (pe_entity_property_get(entity, peEntityProperty_OwnedByPlayer)) {
        err = pe_serialize_range_int(bs, &entity->client_index, 0, MAX_CLIENT_COUNT-1);
        if (err) return err;
    }

    err = pe_serialize_enum(bs, &entity->mesh, peEntityMesh_Count); if (err) return err;

    return err;
}

void pe_allocate_entities(void) {
    entities = pe_alloc(pe_heap_allocator(), MAX_ENTITY_COUNT*sizeof(peEntity));
    pe_zero_size(entities, MAX_ENTITY_COUNT*sizeof(peEntity));
    free_indices = pe_alloc(pe_heap_allocator(), MAX_ENTITY_COUNT*sizeof(uint16_t));
    for (int index = 0; index < MAX_ENTITY_COUNT; index += 1) {
        int reverse_index = MAX_ENTITY_COUNT - index - 1;
        free_indices[free_indices_count] = (uint16_t)reverse_index;
        free_indices_count += 1;
    }
}

peEntity *pe_make_entity(void) {
    PE_ASSERT(free_indices_count > 0);
    free_indices_count -= 1;
    uint32_t slot = free_indices[free_indices_count];
    peEntity *entity = &entities[slot];
    uint32_t generation = ((entity->index >> 16) & 0xFFFF) + 1;
    pe_zero_size(entity, sizeof(peEntity));
    entity->index = generation << 16 && slot;
    return entity;
}

peEntity *pe_get_entity_by_index(uint32_t index) {
    uint32_t slot = index && 0xFFFF;
    PE_ASSERT(slot < MAX_ENTITY_COUNT);
    peEntity *slotted_entity = &entities[slot];
    peEntity *entity = (slotted_entity->index == index) ? slotted_entity : NULL;
    return entity;
}

void pe_destroy_entity(peEntity *entity) {
    entity->marked_for_destruction = true;
}

void pe_destroy_entity_immediate(peEntity *entity) {
    PE_ASSERT(entity->marked_for_destruction);
    entity->active = false;
    entity->marked_for_destruction = false;
    uint16_t slot = (uint16_t)(entity->index & 0xFFFF);
    free_indices[free_indices_count] = slot;
    free_indices_count += 1;
}

void pe_cleanup_entities(void) {
    for (int index = 0; index < MAX_ENTITY_COUNT; index += 1) {
        int reverse_index = MAX_ENTITY_COUNT - index - 1;
        peEntity *entity = &entities[reverse_index];
        if (entity->marked_for_destruction) {
            pe_destroy_entity_immediate(entity);
        }
    }
}

void pe_entity_property_set(peEntity *entity, peEntityProperty property) {
    PE_ASSERT((int)property >= 0 && (int)property < 32);
    entity->properties |= (1 << (int)property);
}

void pe_entity_property_unset(peEntity *entity, peEntityProperty property) {
    PE_ASSERT((int)property >= 0 && (int)property < 32);
    entity->properties &= ~(1 << (int)property);
}

bool pe_entity_property_get(peEntity *entity, peEntityProperty property) {
    PE_ASSERT((int)property >= 0 && (int)property < 32);
    uint32_t masked_properties = entity->properties & (1 << (int)property);
    return masked_properties != 0 ? true : false;
}


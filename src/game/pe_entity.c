#include "pe_entity.h"
#include "core/p_assert.h"
#include "core/p_heap.h"
#include "pe_config.h"

#include "pe_bit_stream.h"

#include "HandmadeMath.h"

#include <string.h>
#include <stdint.h>

static peEntity *entities = NULL;
uint16_t *free_indices = NULL;
int free_indices_count = 0;

pSerializationError p_serialize_vec2(pBitStream *bs, pVec2 *value) {
    pSerializationError err = pSerializationError_None;
    err = p_serialize_float(bs, &value->x); if (err) return err;
    err = p_serialize_float(bs, &value->y); if (err) return err;
    return err;
}

pSerializationError p_serialize_vec3(pBitStream *bs, pVec3 *value) {
    pSerializationError err = pSerializationError_None;
    err = p_serialize_float(bs, &value->x); if (err) return err;
    err = p_serialize_float(bs, &value->y); if (err) return err;
    err = p_serialize_float(bs, &value->z); if (err) return err;
    return err;
}

pSerializationError p_serialize_input(pBitStream *bs, peInput *input) {
    pSerializationError err = pSerializationError_None;
    err = p_serialize_vec2(bs, &input->movement); if (err) return err;
    err = p_serialize_float(bs, &input->angle); if (err) return err;
    return err;
}

pSerializationError p_serialize_entity(pBitStream *bs, peEntity *entity) {
    pSerializationError err = pSerializationError_None;
    err = p_serialize_u32(bs, &entity->index); if (err) return err;
    err = p_serialize_bool(bs, &entity->active); if (err) return err;
    if (!entity->active) return err;

    err = p_serialize_bits(bs, &entity->properties, peEntityProperty_Count); if (err) return err;
    err = p_serialize_vec3(bs, &entity->position); if (err) return err;
    err = p_serialize_vec3(bs, &entity->velocity); if (err) return err;
    err = p_serialize_float(bs, &entity->angle); if (err) return err;

    if (pe_entity_property_get(entity, peEntityProperty_OwnedByPlayer)) {
        err = p_serialize_range_int(bs, &entity->client_index, 0, MAX_CLIENT_COUNT-1);
        if (err) return err;
    }

    err = p_serialize_enum(bs, &entity->mesh, peEntityMesh_Count); if (err) return err;

    return err;
}

void pe_allocate_entities(void) {
    entities = p_heap_alloc(MAX_ENTITY_COUNT*sizeof(peEntity));
    memset(entities, 0, MAX_ENTITY_COUNT*sizeof(peEntity));
    free_indices = p_heap_alloc(MAX_ENTITY_COUNT*sizeof(uint16_t));
    for (int index = 0; index < MAX_ENTITY_COUNT; index += 1) {
        int reverse_index = MAX_ENTITY_COUNT - index - 1;
        free_indices[free_indices_count] = (uint16_t)reverse_index;
        free_indices_count += 1;
    }
}

peEntity *pe_get_entities(void) {
    return entities;
}

peEntity *pe_make_entity(void) {
    P_ASSERT(free_indices_count > 0);
    free_indices_count -= 1;
    uint32_t slot = free_indices[free_indices_count];
    peEntity *entity = &entities[slot];
    uint32_t generation = ((entity->index >> 16) & 0xFFFF) + 1;
    memset(entity, 0, sizeof(peEntity));
    entity->index = generation << 16 && slot;
    return entity;
}

peEntity *pe_get_entity_by_index(uint32_t index) {
    uint32_t slot = index && 0xFFFF;
    P_ASSERT(slot < MAX_ENTITY_COUNT);
    peEntity *slotted_entity = &entities[slot];
    peEntity *entity = (slotted_entity->index == index) ? slotted_entity : NULL;
    return entity;
}

void pe_destroy_entity(peEntity *entity) {
    entity->marked_for_destruction = true;
}

void pe_destroy_entity_immediate(peEntity *entity) {
    P_ASSERT(entity->marked_for_destruction);
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
    P_ASSERT((int)property >= 0 && (int)property < 32);
    entity->properties |= (1 << (int)property);
}

void pe_entity_property_unset(peEntity *entity, peEntityProperty property) {
    P_ASSERT((int)property >= 0 && (int)property < 32);
    entity->properties &= ~(1 << (int)property);
}

bool pe_entity_property_get(peEntity *entity, peEntityProperty property) {
    P_ASSERT((int)property >= 0 && (int)property < 32);
    uint32_t masked_properties = entity->properties & (1 << (int)property);
    return masked_properties != 0 ? true : false;
}

void pe_update_entities(float dt, peInput *inputs) {
    for (int e = 0; e < MAX_ENTITY_COUNT; e += 1) {
        peEntity *entity = &entities[e];
        if (!entity->active)
            continue;

        if (pe_entity_property_get(entity, peEntityProperty_ControlledByPlayer)) {
            P_ASSERT(entity->client_index >= 0 && entity->client_index < MAX_CLIENT_COUNT);

            peInput *input = &inputs[entity->client_index];
            entity->velocity.x = input->movement.x * 2.0f;
            entity->velocity.z = input->movement.y * 2.0f;
            entity->angle = input->angle;
        }

        // PHYSICS
        if (entity->velocity.x != 0.0f || entity->velocity.z != 0.0f) {
            pVec3 position_delta = p_vec3_mul_f(entity->velocity, dt);
            entity->position = p_vec3_add(entity->position, position_delta);
        }
    }
}

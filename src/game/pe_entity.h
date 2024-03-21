#ifndef PE_ENTITY_H
#define PE_ENTITY_H

#include <stdint.h>
#include <stdbool.h>

#include "HandmadeMath.h"

#define MAX_ENTITY_COUNT 32

typedef enum peEntityProperty {
    peEntityProperty_CanCollide,
    peEntityProperty_OwnedByPlayer,
    peEntityProperty_ControlledByPlayer,
    peEntityProperty_Count,
} peEntityProperty;

typedef enum peEntityMesh {
    peEntityMesh_Cube,
    peEntityMesh_Quad,
    peEntityMesh_Count,
} peEntityMesh;

typedef struct peEntity {
    uint32_t index;
    bool active;
    bool marked_for_destruction;
    uint32_t properties;

    HMM_Vec3 position;
    HMM_Vec3 velocity;
    float angle;

    int client_index; // property: OwnedByPlayer

    peEntityMesh mesh;
} peEntity;

typedef struct peInput {
    HMM_Vec2 movement;
    float angle;
} peInput;

struct peBitStream;
enum peSerializationError pe_serialize_vec2(struct peBitStream *bs, HMM_Vec2 *value);
enum peSerializationError pe_serialize_vec3(struct peBitStream *bs, HMM_Vec3 *value);
enum peSerializationError pe_serialize_input(struct peBitStream *bs, peInput *input);
enum peSerializationError pe_serialize_entity(struct peBitStream *bs, peEntity *entity);
void pe_allocate_entities(void);
peEntity *pe_get_entities(void);
peEntity *pe_make_entity(void);
peEntity *pe_get_entity_by_index(uint32_t index);
void pe_destroy_entity(peEntity *entity);
void pe_destroy_entity_immediate(peEntity *entity);
void pe_cleanup_entities(void);
void pe_entity_property_set(peEntity *entity, peEntityProperty property);
void pe_entity_property_unset(peEntity *entity, peEntityProperty property);
bool pe_entity_property_get(peEntity *entity, peEntityProperty property);

void pe_update_entities(float dt, peInput *inputs);



#endif // PE_ENTITY_H
#ifndef PE_ENTITY_H
#define PE_ENTITY_H

#include <stdint.h>
#include <stdbool.h>

#include "math/p_math.h"

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

    pVec3 position;
    pVec3 velocity;
    float angle;

    int client_index; // property: OwnedByPlayer

    peEntityMesh mesh;
} peEntity;

typedef struct peInput {
    pVec2 movement;
    float angle;
} peInput;

struct pBitStream;
enum pSerializationError p_serialize_vec2(struct pBitStream *bs, pVec2 *value);
enum pSerializationError p_serialize_vec3(struct pBitStream *bs, pVec3 *value);
enum pSerializationError p_serialize_input(struct pBitStream *bs, peInput *input);
enum pSerializationError p_serialize_entity(struct pBitStream *bs, peEntity *entity);
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

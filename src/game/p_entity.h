#ifndef PE_ENTITY_H
#define PE_ENTITY_H

#include <stdint.h>
#include <stdbool.h>

#include "math/p_math.h"

#include "HandmadeMath.h"

#define MAX_ENTITY_COUNT 32

typedef enum pEntityProperty {
    pEntityProperty_CanCollide,
    pEntityProperty_OwnedByPlayer,
    pEntityProperty_ControlledByPlayer,
    pEntityProperty_Count,
} pEntityProperty;

typedef enum pEntityMesh {
    pEntityMesh_Cube,
    pEntityMesh_Quad,
    pEntityMesh_Count,
} pEntityMesh;

typedef struct pEntity {
    uint32_t index;
    bool active;
    bool marked_for_destruction;
    uint32_t properties;

    pVec3 position;
    pVec3 velocity;
    float angle;

    int client_index; // property: OwnedByPlayer

    pEntityMesh mesh;
} pEntity;

typedef struct pInput {
    pVec2 movement;
    float angle;
} pInput;

struct pBitStream;
enum pSerializationError p_serialize_vec2(struct pBitStream *bs, pVec2 *value);
enum pSerializationError p_serialize_vec3(struct pBitStream *bs, pVec3 *value);
enum pSerializationError p_serialize_input(struct pBitStream *bs, pInput *input);
enum pSerializationError p_serialize_entity(struct pBitStream *bs, pEntity *entity);
void p_allocate_entities(void);
pEntity *p_get_entities(void);
pEntity *p_make_entity(void);
pEntity *p_get_entity_by_index(uint32_t index);
void p_destroy_entity(pEntity *entity);
void p_destroy_entity_immediate(pEntity *entity);
void p_cleanup_entities(void);
void p_entity_property_set(pEntity *entity, pEntityProperty property);
void p_entity_property_unset(pEntity *entity, pEntityProperty property);
bool p_entity_property_get(pEntity *entity, pEntityProperty property);

void p_update_entities(float dt, pInput *inputs);

#endif // PE_ENTITY_H

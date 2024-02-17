#ifndef PE_PROFILE_H
#define PE_PROFILE_H

#include <stdint.h>

typedef enum peProfileRegion {
    peProfileRegion_GameLoop,
    peProfileRegion_ModelDraw,
    peProfileRegion_ModelDraw_ConcatenateAnimationJoints,
    peProfileRegion_ModelDraw_CalculateMatrices,
    peProfileRegion_ModelDraw_AssignMatrices,
    peProfileRegion_ModelDraw_BindTexture,
    peProfileRegion_ModelDraw_DrawArray,
    peProfileRegion_FrameEnd,
    peProfileRegion_Count
} peProfileRegion;

typedef struct peProfileRecord {
    uint64_t start_time;
    uint64_t time_spent;
    uint32_t count;
} peProfileRecord;

void pe_profile_reset(void);
void pe_profile_region_begin(peProfileRegion profile_region);
void pe_profile_region_end(peProfileRegion profile_region);
peProfileRecord pe_profile_record_get(peProfileRegion profile_region);

#endif // PE_PROFILE_H
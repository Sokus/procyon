#include "pe_profile.h"

#include "pe_core.h"
#include "pe_time.h"

static peProfileRecord profile_records[peProfileRegion_Count] = {0};

void pe_profile_reset(void) {
    for (int p = 0; p < peProfileRegion_Count; p += 1) {
        profile_records[p].time_spent = 0;
        profile_records[p].count = 0;
    }
}

void pe_profile_region_begin(peProfileRegion profile_region) {
    if (PE_ALWAYS(profile_region >= 0 && profile_region < peProfileRegion_Count)) {
        profile_records[profile_region].start_time = pe_time_now();
    }
}

void pe_profile_region_end(peProfileRegion profile_region) {
    if (PE_ALWAYS(profile_region >= 0 && profile_region < peProfileRegion_Count)) {
        uint64_t start_time = profile_records[profile_region].start_time;
        uint64_t elapsed_time = pe_time_since(start_time);
        profile_records[profile_region].time_spent += elapsed_time;
        profile_records[profile_region].count += 1;
    }
}

peProfileRecord pe_profile_record_get(peProfileRegion profile_region) {
    peProfileRecord result = {0};
    if (PE_ALWAYS(profile_region >= 0 && profile_region < peProfileRegion_Count)) {
        result = profile_records[profile_region];
    }
    return result;
}
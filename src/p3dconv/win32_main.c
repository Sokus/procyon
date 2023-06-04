#include "pe_core.h"

#define M3D_IMPLEMENTATION
#include "m3d.h"
#include "HandmadeMath.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>

typedef struct peFileContents {
    peAllocator allocator;
    void *data;
    size_t size;
} peFileContents;

peFileContents pe_file_read_contents(peAllocator allocator, char *file_path, bool zero_terminate) {
    peFileContents result = {0};
    result.allocator = allocator;

    HANDLE file_handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        return result;
    }

    DWORD file_size;
    if ((file_size = GetFileSize(file_handle, &file_size)) == INVALID_FILE_SIZE) {
        CloseHandle(file_handle);
        return result;
    }

    size_t total_size = zero_terminate ? file_size + 1 : file_size;
    void *data = pe_alloc(allocator, total_size);
    if (data == NULL) {
        CloseHandle(file_handle);
        return result;
    }

    DWORD bytes_read = 0;
    if (!ReadFile(file_handle, data, file_size, &bytes_read, NULL)) {
        CloseHandle(file_handle);
        pe_free(allocator, data);
        return result;
    }
    PE_ASSERT(bytes_read == file_size);
    CloseHandle(file_handle);

    result.data = data;
    result.size = total_size;
    if (zero_terminate) {
        ((uint8_t*)data)[file_size] = 0;
    }

    return result;
}

static char *pe_m3d_current_path = NULL;
static peAllocator pe_m3d_allocator = {0};

unsigned char *pe_m3d_read_callback(char *filename, unsigned int *size) {
    char file_path[512] = "\0";
    int file_path_length = 0;
    if (pe_m3d_current_path != NULL) {
        int one_past_last_slash_offset = 0;
        for (int i = 0; i < PE_COUNT_OF(file_path)-1; i += 1) {
            if (pe_m3d_current_path[i] == '\0') break;
            if (pe_m3d_current_path[i] == '/') {
                one_past_last_slash_offset = i + 1;
            }
        }
        memcpy(file_path, pe_m3d_current_path, one_past_last_slash_offset);
        file_path_length += one_past_last_slash_offset;
    }
    strcat_s(file_path+file_path_length, sizeof(file_path)-file_path_length-1, filename);

    printf("M3D read callback: %s\n", file_path);
    peFileContents file_contents = pe_file_read_contents(pe_m3d_allocator, file_path, false);
    *size = (unsigned int)file_contents.size;
    return file_contents.data;
}

void pe_m3d_free_callback(void *buffer) {
    pe_free(pe_m3d_allocator, buffer);
}

typedef struct peColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} peColor;

#define PE_COLOR_WHITE (peColor){ 255, 255, 255, 255 }

HMM_Vec4 pe_color_to_vec4(peColor color) {
    HMM_Vec4 vec4 = {
        .R = (float)color.r / 255.0f,
        .G = (float)color.g / 255.0f,
        .B = (float)color.b / 255.0f,
        .A = (float)color.a / 255.0f,
    };
    return vec4;
}

peColor pe_color_uint32(uint32_t color_uint32) {
    peColor color = {
        .r = (uint8_t)((color_uint32 >>  0) & 0xFF),
        .g = (uint8_t)((color_uint32 >>  8) & 0xFF),
        .b = (uint8_t)((color_uint32 >> 16) & 0xFF),
        .a = (uint8_t)((color_uint32 >> 24) & 0xFF),
    };
    return color;
}

void pe_p3d_convert(char *source, char *destination) {
    peArena temp_arena;
    pe_arena_init_from_allocator(&temp_arena, pe_heap_allocator(), PE_MEGABYTES(32));
    peAllocator temp_allocator = pe_arena_allocator(&temp_arena);
    pe_m3d_allocator = temp_allocator;
    pe_m3d_current_path = source;

    peFileContents m3d_file_contents = pe_file_read_contents(temp_allocator, source, false);
    m3d_t *m3d = m3d_load(m3d_file_contents.data, /*pe_m3d_read_callback*/NULL, pe_m3d_free_callback, NULL);

    struct peTempVertexInfo {
        HMM_Vec3 position;
        HMM_Vec3 normal;
        HMM_Vec2 uv;
        peColor color;

        uint32_t material_id;
        uint32_t vertex_index_all;
        uint32_t vertex_index_mesh;
    };
    struct peTempVertexInfo *temp_vertex_info = pe_alloc(temp_allocator, m3d->numvertex*sizeof(struct peTempVertexInfo));
    for (M3D_INDEX i = 0; i < m3d->numvertex; i += 1) {
        temp_vertex_info[i].material_id = M3D_UNDEF;
        temp_vertex_info[i].vertex_index_all = UINT32_MAX;
        temp_vertex_info[i].vertex_index_mesh = UINT32_MAX;
    }

    struct peTempMeshInfo {
        unsigned int num_vertex;
        uint32_t num_index;
        uint32_t index_offset;
        bool has_diffuse_color;
        peColor diffuse_color;
        bool has_texture_coordinates;
        bool has_diffuse_texture;
        char *diffuse_texture_name;
    };
    struct peTempMeshInfo *temp_mesh_info = pe_alloc(temp_allocator, (m3d->nummaterial+1)*sizeof(struct peTempMeshInfo));
    pe_zero_size(temp_mesh_info, (m3d->nummaterial+1)*sizeof(struct peTempMeshInfo));

    uint32_t num_vertex = 0;

    for (M3D_INDEX f = 0; f < m3d->numface; f += 1) {
        M3D_INDEX mesh_index = (m3d->face[f].materialid == M3D_UNDEF ? m3d->nummaterial : m3d->face[f].materialid);
        temp_mesh_info[mesh_index].num_index += 3;
        for (M3D_INDEX m = mesh_index+1; m < m3d->nummaterial+1; m += 1) {
            temp_mesh_info[m].index_offset += 3;
        }
        for (int v = 0; v < 3; v += 1) {
            M3D_INDEX vert_index = m3d->face[f].vertex[v];
            temp_vertex_info[vert_index].position.X = m3d->vertex[vert_index].x;
            temp_vertex_info[vert_index].position.Y = m3d->vertex[vert_index].y;
            temp_vertex_info[vert_index].position.Z = m3d->vertex[vert_index].z;
            M3D_INDEX norm_index = m3d->face[f].normal[v];
            temp_vertex_info[vert_index].normal.X = m3d->vertex[norm_index].x;
            temp_vertex_info[vert_index].normal.Y = m3d->vertex[norm_index].y;
            temp_vertex_info[vert_index].normal.Z = m3d->vertex[norm_index].z;
            if (m3d->face[f].texcoord[0] != M3D_UNDEF) {
			    temp_mesh_info[mesh_index].has_texture_coordinates = true;
                M3D_INDEX tex_index = m3d->face[f].texcoord[v];
                temp_vertex_info[vert_index].uv.X = m3d->tmap[tex_index].u;
                temp_vertex_info[vert_index].uv.Y = 1.0f - m3d->tmap[tex_index].v;
            }
			temp_vertex_info[vert_index].material_id = mesh_index;
            if (temp_vertex_info[vert_index].vertex_index_all == UINT32_MAX) {
                temp_vertex_info[vert_index].vertex_index_all = num_vertex;
                num_vertex += 1;
            }
			if (temp_vertex_info[vert_index].vertex_index_mesh == UINT32_MAX) {
				temp_vertex_info[vert_index].vertex_index_mesh = temp_mesh_info[mesh_index].num_vertex;
				temp_mesh_info[mesh_index].num_vertex += 1;
			}
        }
    }

    for (M3D_INDEX m = 0; m < m3d->nummaterial; m += 1) {
        for (M3D_INDEX p = 0; p < m3d->material[m].numprop; p += 1) {
            switch (m3d->material[m].prop[p].type) {
                case m3dp_Kd: {
                    temp_mesh_info[m].has_diffuse_color = true;
                    temp_mesh_info[m].diffuse_color = pe_color_uint32(m3d->material[m].prop[p].value.color);
                } break;

                case m3dp_map_Kd: { /* diffuse map */
                    m3dtx_t *m3d_texture = &m3d->texture[m3d->material[m].prop[p].value.textureid];
                    temp_mesh_info[m].diffuse_texture_name = m3d_texture->name;
                } break;

                // UNUSED PROPERTIES
                case m3dp_Ka: { uint32_t ambient = m3d->material[m].prop[p].value.color; } break;
                case m3dp_Ks: { uint32_t specular = m3d->material[m].prop[p].value.color; } break;
                case m3dp_Pr: { float roughness = m3d->material[m].prop[p].value.fnum; } break;
                case m3dp_Pm: { float metallic = m3d->material[m].prop[p].value.fnum; } break;
                case m3dp_Ni: { float refraction = m3d->material[m].prop[p].value.fnum; } break;

                // IGNORED PROPERTIES
                case m3dp_Km:     // [6] bump
                case m3dp_d:      // [7] dissolve (obsolete)
                case m3dp_il:     // [8] illumination model
                case m3dp_map_Km: // [134] bump map
                case m3dp_map_D:  // [135] disolve map (obsolete)
                    break;

                default: printf("unknown property: %u\n", m3d->material[m].prop[p].type); break;
            }
        }
    }

    HANDLE dest_file_handle = CreateFileA(destination, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (dest_file_handle == INVALID_HANDLE_VALUE) {
        printf("could not create file\n");
        return;
    }

    // scale;
    WriteFile(dest_file_handle, &m3d->scale, sizeof(m3d->scale), NULL, NULL);

    // num_vertex
    WriteFile(dest_file_handle, &num_vertex, sizeof(num_vertex), NULL, NULL);

    // position
    for (M3D_INDEX v = 0; v < m3d->numvertex; v += 1) {
        if (temp_vertex_info[v].vertex_index_all != UINT32_MAX) {
            int16_t write_position_x = (int16_t)(temp_vertex_info[v].position.X * (float)INT16_MAX);
            WriteFile(dest_file_handle, &write_position_x, sizeof(int16_t), NULL, NULL);
            int16_t write_position_y = (int16_t)(temp_vertex_info[v].position.Y * (float)INT16_MAX);
            WriteFile(dest_file_handle, &write_position_y, sizeof(int16_t), NULL, NULL);
            int16_t write_position_z = (int16_t)(temp_vertex_info[v].position.Z * (float)INT16_MAX);
            WriteFile(dest_file_handle, &write_position_z, sizeof(int16_t), NULL, NULL);
        }
    }

    // normal
    for (M3D_INDEX v = 0; v < m3d->numvertex; v += 1) {
        if (temp_vertex_info[v].vertex_index_all != UINT32_MAX) {
            int16_t write_normal_x = (int16_t)(temp_vertex_info[v].normal.X * (float)INT16_MAX);
            WriteFile(dest_file_handle, &write_normal_x, sizeof(int16_t), NULL, NULL);
            int16_t write_normal_y = (int16_t)(temp_vertex_info[v].normal.Y * (float)INT16_MAX);
            WriteFile(dest_file_handle, &write_normal_y, sizeof(int16_t), NULL, NULL);
            int16_t write_normal_z = (int16_t)(temp_vertex_info[v].normal.Z * (float)INT16_MAX);
            WriteFile(dest_file_handle, &write_normal_z, sizeof(int16_t), NULL, NULL);
        }
    }

    // texcoord
    for (M3D_INDEX v = 0; v < m3d->numvertex; v += 1) {
        if (temp_vertex_info[v].vertex_index_all != UINT32_MAX) {
            int16_t write_texcoord_x = (int16_t)(temp_vertex_info[v].uv.X * (float)INT16_MAX);
            WriteFile(dest_file_handle, &write_texcoord_x, sizeof(int16_t), NULL, NULL);
            int16_t write_texcoord_y = (int16_t)(temp_vertex_info[v].uv.Y * (float)INT16_MAX);
            WriteFile(dest_file_handle, &write_texcoord_y, sizeof(int16_t), NULL, NULL);
        }
    }

    // color
    for (M3D_INDEX v = 0; v < m3d->numvertex; v += 1) {
        if (temp_vertex_info[v].vertex_index_all != UINT32_MAX) {
            WriteFile(dest_file_handle, &temp_vertex_info[v].color, sizeof(peColor), NULL, NULL);
        }
    }

    // num_index
    {
        uint32_t write_num_index = 3 * m3d->numface;
        WriteFile(dest_file_handle, &write_num_index, sizeof(uint32_t), NULL, NULL);
    }

    // index
    for (M3D_INDEX v = 0; v < m3d->numvertex; v += 1) {
        if (temp_vertex_info[v].vertex_index_all != UINT32_MAX) {
            WriteFile(dest_file_handle, &temp_vertex_info[v].vertex_index_all, sizeof(uint32_t), NULL, NULL);
        }
    }

    // num_meshes
    uint16_t write_num_meshes = (uint16_t)(temp_mesh_info[m3d->nummaterial].num_index > 0 ? m3d->nummaterial+1 : m3d->nummaterial);
    WriteFile(dest_file_handle, &write_num_meshes, sizeof(uint16_t), NULL, NULL);

    for (uint16_t m = 0; m < write_num_meshes; m += 1) {
        // num_index
        WriteFile(dest_file_handle, &temp_mesh_info[m].num_index, sizeof(uint32_t), NULL, NULL);

        // index offset
        WriteFile(dest_file_handle, &temp_mesh_info[m].index_offset, sizeof(uint32_t), NULL, NULL);

        // has diffuse color?
        uint8_t write_has_diffuse = temp_mesh_info[m].has_diffuse_color ? 1 : 0;
        WriteFile(dest_file_handle, &write_has_diffuse, sizeof(uint8_t), NULL, NULL);
        if (temp_mesh_info[m].has_diffuse_color) {
            // diffuse color
            WriteFile(dest_file_handle, &temp_mesh_info[m].diffuse_color, sizeof(peColor), NULL, NULL);
        }

        // has diffuse texture?
        uint8_t write_has_diffuse_texture = temp_mesh_info[m].has_diffuse_texture ? 1 : 0;
        WriteFile(dest_file_handle, &write_has_diffuse_texture, sizeof(uint8_t), NULL, NULL);
        if (temp_mesh_info[m].has_diffuse_texture) {
            if (temp_mesh_info[m].diffuse_texture_name != NULL) {
                uint8_t diffuse_texture_name_length = (uint8_t)strnlen(temp_mesh_info[m].diffuse_texture_name, UINT8_MAX-1);
                WriteFile(dest_file_handle, &diffuse_texture_name_length, sizeof(diffuse_texture_name_length), NULL, NULL);
                WriteFile(dest_file_handle, temp_mesh_info[m].diffuse_texture_name, (diffuse_texture_name_length+1)*sizeof(char), NULL, NULL);
            }
        }
    }

    CloseHandle(dest_file_handle);


    m3d_free(m3d);
    pe_arena_free(&temp_arena);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("not enough arguments\n");
        return -1;
    }

    pe_p3d_convert(argv[1], argv[2]);

    return 0;
}
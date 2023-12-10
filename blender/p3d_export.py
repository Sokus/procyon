bl_info = {
    "name": "Procyon Asset Exporter",
    "author": "Sokus",
    "version": (0, 0, 1),
    "blender": (3, 2, 0),
    "location": "Properties > Object > Export",
    "description": "One-click export game asset files.",
    "category": "Export"}

import bpy
import bmesh
import struct
import mathutils
import sys
from copy import (copy)
from pathlib import Path
from bpy.utils import (register_class, unregister_class)
from bpy.types import (Panel, PropertyGroup)
from bpy.props import (StringProperty, BoolProperty, IntProperty, FloatProperty, FloatVectorProperty, EnumProperty, PointerProperty)
from bpy_extras.io_utils import (axis_conversion)
from bpy_extras.node_shader_utils import (PrincipledBSDFWrapper)

UINT8_MAX = 255
INT16_MAX = 32767
INT16_MIN = -32768
UINT16_MAX = 65535

class Vertex:
    def __init__(self, position, uv, normal, color, joint_indices, joint_weights):
        self.position = position
        self.uv = uv
        self.normal = normal
        self.color = color
        self.joint_indices = joint_indices
        self.joint_weights = joint_weights

    def __eq__(self, other):
        return self.__dict__ == other.__dict__
    def __hash__(self):
        return hash(self.position.x)

class Material:
    def __init__(self):
        self.diffuse_color = []
        self.diffuse_image = None

class Mesh:
    def __init__(self):
        self.material_index = -1
        self.subskeleton_index = -1
        self.vertices = {}
        self.indices = []
        self.index_offset = 0
        self.vertex_offset = 0

class SkeletonJoint:
    def __init__(self, name, parent_index, inverse_model_space_pose):
        self.name = name
        self.parent_index = parent_index
        self.inverse_model_space_pose = inverse_model_space_pose

class AnimationJoint:
    def __init__(self, position, rotation, scale):
        self.position = position
        self.rotation = rotation
        self.scale = scale

class AnimationFrame:
    def __init__(self):
        self.joints = []

class Animation:
    def __init__(self, name):
        self.name = name
        self.frames = []

class Skeleton:
    def __init__(self):
        self.joints = []
        self.animations = []

class ProcyonData:
    def __init__(self):
        self.scale = 1.0
        self.num_vertex = 0
        self.num_index = 0
        self.meshes = []
        self.materials = []
        self.joints = []
        self.animations = []
        self.bone_groups = []

def write_int32(file, value):
    use_ascii = bpy.context.scene.export_properties.use_ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("i", value))

def write_uint32(file, value):
    use_ascii = bpy.context.scene.export_properties.use_ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("I", value))

def write_int16(file, value):
    use_ascii = bpy.context.scene.export_properties.use_ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("h", value))

def write_uint16(file, value):
    use_ascii = bpy.context.scene.export_properties.use_ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("H", value))

def write_uint8(file, value):
    use_ascii = bpy.context.scene.export_properties.use_ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("B", value))

def write_float(file, value):
    use_ascii = bpy.context.scene.export_properties.use_ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("f", value))

def write_bool(file, value):
    use_ascii = bpy.context.scene.export_properties.use_ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("?", value))

def write_string(file, text):
    use_ascii = bpy.context.scene.export_properties.use_ascii
    if use_ascii: file.write(text)
    else: file.write(text.encode('ascii'))

def normalize_joint_weights(weights):
    total_weight = sum(weights)
    result = []
    if total_weight != 0:
        for weight in weights:
            result.append(weight / total_weight)
    return result

def triangulate_mesh(mesh):
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(mesh)

def float_to_int16(value, a=-1.0, b=1.0):
    float_negative_one_to_one = 2.0 * ((value - a) / (b - a)) - 1.0
    if float_negative_one_to_one >= 0:
        return int(float_negative_one_to_one * float(INT16_MAX))
    else:
        return int(float_negative_one_to_one * float(abs(INT16_MIN)))

def float_to_uint16(value, a=-1.0, b=1.0):
    float_zero_to_one = (value - a) / (b - a)
    return int(float_zero_to_one * float(UINT16_MAX))

def color_to_uint32(value):
    r = int(value[0] * 255)
    g = int(value[1] * 255)
    b = int(value[2] * 255)
    a = int(value[3] * 255)
    return r | g << 8 | b << 16 | a << 24

def get_selected_mesh_objects():
    mesh_list = []
    for object in bpy.context.selected_objects:
        if object.type == "MESH":
            mesh_list.append(object)
    return mesh_list

def get_selected_armature():
    for object in bpy.context.selected_objects:
        if object.type == "ARMATURE":
            return object
    return 0

def get_axis_mapping_matrix():
    return axis_conversion("-Y", "Z", bpy.context.scene.export_properties.forward_axis, bpy.context.scene.export_properties.up_axis).to_4x4()

def set_armature_position(armature, position):
    armature.data.pose_position = position
    armature.data.update_tag()
    bpy.context.scene.frame_set(bpy.context.scene.frame_current)

def join_arrays(a, b):
    new_array = a.copy()
    new_array.extend(x for x in b if x not in new_array)
    return new_array

def write_proc(operator, context, file_type):
    procyon_data = ProcyonData()
    print("Exporting", file_type)

    armature = get_selected_armature()
    transform_matrix = get_axis_mapping_matrix()

    if armature:
        original_armature_position = armature.data.pose_position
        original_action = armature.animation_data.action
        original_frame = bpy.context.scene.frame_current
        set_armature_position(armature, "REST")

    objects = get_selected_mesh_objects()
    if len(objects) > 0:
        scene_with_applied_modifiers = bpy.context.evaluated_depsgraph_get()
        materials = {}
        meshes = []

        dummy_color = [1.0, 1.0, 1.0, 1.0]
        dummy_material = bpy.data.materials.new("Dummy")
        dummy_material.diffuse_color = dummy_color

        for object in objects:
            mesh = object.evaluated_get(scene_with_applied_modifiers).to_mesh(preserve_all_data_layers=True, depsgraph=bpy.context.evaluated_depsgraph_get())
            mesh.transform(transform_matrix @ object.matrix_world)
            triangulate_mesh(mesh)
            mesh.calc_normals_split()

            for polygon in mesh.polygons:
                if len(polygon.loop_indices) == 3:
                    use_materials = bpy.context.scene.export_properties.use_materials
                    if  use_materials and len(mesh.materials) > 0:
                        material = mesh.materials[polygon.material_index]
                    else:
                        material = dummy_material

                    material_index = 0
                    if materials.get(material) == None:
                        material_bsdf = PrincipledBSDFWrapper(material)
                        if material_bsdf == None:
                            operator.report({"ERROR"}, "Material '", material.name, "' does not use PrincipledBSDF surface, not parsing.")
                            return False

                        materials[material] = Material()

                        if material_bsdf.base_color and len(material_bsdf.base_color) > 3:
                            alpha = material_bsdf.base_color[3]
                        else:
                            alpha = material_bsdf.alpha
                        materials[material].diffuse_color = [material_bsdf.base_color[0], material_bsdf.base_color[1], material_bsdf.base_color[2], alpha]

                        base_color_socket = material_bsdf.node_principled_bsdf.inputs['Base Color']
                        if base_color_socket.links:
                            materials[material].diffuse_image = base_color_socket.links[0].from_node.image
                            if len(base_color_socket.links) > 1:
                                print(f"WARNING: base color socket has {len(base_color_socket.links)} links, only the first one is processed!")
                    material_index = list(materials.keys()).index(material)

                    mesh_index = -1
                    for index, p_mesh in enumerate(meshes):
                        if p_mesh.material_index != -1 and p_mesh.material_index == material_index:
                            mesh_index = index
                            break

                    if not mesh_index >= 0:
                        mesh_index = len(meshes)
                        meshes.append(Mesh())
                        meshes[mesh_index].material_index = material_index

                    # Get mesh vertices, indices and joint info
                    face_indices = []
                    for loop_index in polygon.loop_indices:
                        loop = mesh.loops[loop_index]

                        joint_indices = []
                        joint_weights = []
                        if armature:
                            for g, group in enumerate(mesh.vertices[loop.vertex_index].groups):
                                # TODO: sort by weight and get the first 4 weights
                                if g < 4:
                                    bone_name = object.vertex_groups[group.group].name
                                    joint_indices.append(armature.data.bones.find(bone_name))
                                    joint_weights.append(group.weight)

                        uv = mesh.uv_layers.active.data[loop.index].uv

                        vertex = Vertex(
                            mesh.vertices[loop.vertex_index].undeformed_co.copy(), # position
                            mathutils.Vector((uv.x, 1.0-uv.y)), # uv
                            loop.normal.copy(), # normal
                            dummy_color, # color
                            joint_indices,
                            normalize_joint_weights(joint_weights)
                        )

                        if meshes[mesh_index].vertices.get(vertex) == None:
                            meshes[mesh_index].vertices[vertex] = len(meshes[mesh_index].vertices)
                        meshes[mesh_index].indices.append(meshes[mesh_index].vertices[vertex])

        min_vector = [sys.float_info.max, sys.float_info.max, sys.float_info.max]
        max_vector = [sys.float_info.min, sys.float_info.min, sys.float_info.min]
        for mesh in meshes:
            for vertex in mesh.vertices.keys():
                for e in range(0, 3):
                    if vertex.position[e] < min_vector[e]:
                        min_vector[e] = vertex.position[e]
                    if vertex.position[e] > max_vector[e]:
                        max_vector[e] = vertex.position[e]
        procyon_data.scale = max(abs(min(min_vector)), abs(max(max_vector)))

        index_offset = 0
        vertex_offset = 0
        for mesh in meshes:
            procyon_data.num_vertex += len(mesh.vertices)
            procyon_data.num_index += len(mesh.indices)
            mesh.index_offset = index_offset
            mesh.vertex_offset = vertex_offset
            index_offset += len(mesh.indices)
            vertex_offset += len(mesh.vertices)

        procyon_data.materials = list(materials.values())
        procyon_data.meshes = meshes

    if armature:
        set_armature_position(armature, "POSE")

        for bone in armature.data.bones:
            parent_index = armature.data.bones.find(bone.parent.name) if bone.parent else -1
            inverse_model_space_pose = (transform_matrix @ bone.matrix_local).inverted()
            skeleton_joint = SkeletonJoint(bone.name, parent_index,  inverse_model_space_pose)
            procyon_data.joints.append(skeleton_joint)

        for action in bpy.data.actions:
            # Ignore animations with name ending with .001, .002 etc
            if len(action.name) >= 4 and action.name[-4] == '.' and action.name[-3:].isdigit():
                continue
            p_animation = Animation(action.name)
            armature.animation_data.action = action
            start_frame = int(action.frame_range.x)
            end_frame = int(action.frame_range.y)
            for frame in range(start_frame, end_frame+1):
                p_frame = AnimationFrame()
                bpy.context.scene.frame_set(frame)
                for b, bone in enumerate(armature.pose.bones):
                    parent_space_pose = mathutils.Matrix.Identity(4)
                    if bone.parent:
                        parent_space_pose = bone.parent.matrix.inverted() @ bone.matrix
                    else:
                        parent_space_pose = transform_matrix @ bone.matrix
                    translation = parent_space_pose.to_translation()
                    rotation = parent_space_pose.to_quaternion()
                    scale = parent_space_pose.to_scale()
                    animation_joint = AnimationJoint(translation, rotation, scale)
                    p_frame.joints.append(animation_joint)
                p_animation.frames.append(p_frame)
            procyon_data.animations.append(p_animation)
        # Change armature back to the pose it was in.
        bpy.context.scene.frame_set(original_frame)
        armature.animation_data.action = original_action
        set_armature_position(armature, original_armature_position)

        if file_type == "PP3D":
            bone_connections = []
            for mesh in procyon_data.meshes:
                for f in range(int(len(mesh.indices)/3)):
                    triangle_bone_indices = []
                    for index in mesh.indices[(3*f):(3*f+3)]:
                        vertex = list(mesh.vertices.keys())[index]
                        for w, weight in enumerate(vertex.joint_weights):
                            bone_index = vertex.joint_indices[w]
                            if weight > 0.0 and bone_index != -1 and bone_index not in triangle_bone_indices:
                                triangle_bone_indices.append(bone_index)
                        triangle_bone_indices.sort()
                    if not any(connection==triangle_bone_indices for connection in bone_connections):
                        bone_connections.append(triangle_bone_indices)

            bone_connections.extend([b] for b in range(len(procyon_data.joints)) if not any(b in connection for connection in bone_connections))

            def remove_subsets(array_of_arrays):
                new_array = []
                for a_index, a_subarr in enumerate(array_of_arrays):
                    is_subset = False
                    for b_index, b_subarr in enumerate(array_of_arrays):
                        if a_index != b_index and all(x in b_subarr for x in a_subarr):
                            if len(a_subarr) != len(b_subarr) or a_index >= b_index:
                                is_subset = True
                    if not is_subset:
                        new_array.append(a_subarr)
                return new_array
            bone_connections = remove_subsets(bone_connections)

            bone_connections = sorted(bone_connections, key=lambda x: x[:])

            while len(bone_connections) > 0:
                best_candidate_found = False
                best_candidate_score = 0
                best_candidate_length = 0
                best_candidate_index = 0
                best_candidate_bin_index = 0

                for b, bin in enumerate(procyon_data.bone_groups):
                    for g, group in enumerate(bone_connections):
                        new_bin = join_arrays(bin, group)
                        if len(new_bin) > 8: continue

                        score = 1 - (len(new_bin)-len(bin)) / len(group)
                        if (not best_candidate_found or
                            score > best_candidate_score or
                            score == best_candidate_score and len(group)>best_candidate_length):

                            best_candidate_found = True
                            best_candidate_score = score
                            best_candidate_length = len(group)
                            best_candidate_index = g
                            best_candidate_bin_index = b

                if not best_candidate_found:
                    procyon_data.bone_groups.append([])
                    best_candidate_length = 0
                    best_candidate_bin_index = len(procyon_data.bone_groups)-1
                    for g, group in enumerate(bone_connections):
                        if len(group) > best_candidate_length and len(group) <= 8:
                            best_candidate_found = True
                            best_candidate_length = len(group)
                            best_candidate_index = g

                if not best_candidate_found:
                    operator.report({"ERROR"}, "Could not find a bone group with size equal or less than 8, aborting.")
                    return False

                procyon_data.bone_groups[best_candidate_bin_index] = sorted(join_arrays(procyon_data.bone_groups[best_candidate_bin_index], bone_connections[best_candidate_index]))
                bone_connections.pop(best_candidate_index)
            procyon_data.bone_groups = sorted(procyon_data.bone_groups, key=lambda x: x[:])

            new_meshes = []
            for mesh in meshes:
                for f in range(int(len(mesh.indices)/3)):
                    triangle_bone_indices = []
                    triangle_bone_weights = []
                    for index in mesh.indices[(3*f):(3*f+3)]:
                        vertex = list(mesh.vertices.keys())[index]
                        for w, weight in enumerate(vertex.joint_weights):
                            bone_index = vertex.joint_indices[w]
                            if weight > 0.0 and bone_index != -1 and bone_index not in triangle_bone_indices:
                                triangle_bone_indices.append(bone_index)
                    triangle_bone_indices.sort()

                    # FIXME: this shouldn't actually be enforced
                    assert(len(triangle_bone_indices) > 0) # all vertices must be controlled by a bone

                    subskeleton_index = -1
                    for group_index, group in enumerate(procyon_data.bone_groups):
                        if all(bone_index in group for bone_index in triangle_bone_indices):
                            subskeleton_index = group_index

                    assert(subskeleton_index >= 0) # at this point it should always be possible to find a suitable skeleton

                    mesh_index = -1
                    for m, new_mesh in enumerate(new_meshes):
                        if new_mesh.material_index == mesh.material_index and new_mesh.subskeleton_index == subskeleton_index:
                            mesh_index = m
                            break
                    if not mesh_index >= 0:
                        mesh_index = len(new_meshes)
                        new_meshes.append(Mesh())
                        new_meshes[mesh_index].material_index = mesh.material_index
                        new_meshes[mesh_index].subskeleton_index = subskeleton_index

                    for index in mesh.indices[(3*f):(3*f+3)]:
                        old_vertex = list(mesh.vertices.keys())[index]
                        new_vertex = copy(old_vertex)

                        new_vertex.joint_indices = []
                        new_vertex.joint_weights = []
                        for bone_index in procyon_data.bone_groups[subskeleton_index]:
                            if bone_index in old_vertex.joint_indices:
                                vertex_joint_index = old_vertex.joint_indices.index(bone_index)
                                new_vertex.joint_indices.append(bone_index)
                                new_vertex.joint_weights.append(old_vertex.joint_weights[vertex_joint_index])
                            else:
                                new_vertex.joint_indices.append(-1)
                                new_vertex.joint_weights.append(0.0)

                        if new_meshes[mesh_index].vertices.get(new_vertex) == None:
                            new_meshes[mesh_index].vertices[new_vertex] = len(new_meshes[mesh_index].vertices)
                        new_meshes[mesh_index].indices.append(new_meshes[mesh_index].vertices[new_vertex])
            new_meshes = sorted(new_meshes, key=lambda m: (m.subskeleton_index, m.material_index))
            procyon_data.meshes = new_meshes

    if bpy.context.scene.export_properties.use_ascii:
        file_write_mode = "w"
    else:
        file_write_mode = "wb"

    if file_type == "P3D":
        file_path = Path(bpy.path.abspath(context.scene.export_properties.standard_path))
        file = open(str(file_path), file_write_mode)
        write_static_info_header(file, file_type, procyon_data)
        write_mesh_info(file, file_type, file_path, procyon_data)
        write_animation_info(file, file_type, procyon_data)
        write_mesh_data(file, file_type, procyon_data)
        write_bone_parent_indices(file, file_type, procyon_data)
        write_inverse_model_space_matrix(file, file_type, procyon_data)
        write_animation_data(file, file_type, procyon_data)
    elif file_type == "PP3D":
        file_path = Path(bpy.path.abspath(context.scene.export_properties.portable_path))
        file = open(str(file_path), file_write_mode)
        write_static_info_header(file, file_type, procyon_data)
        write_mesh_info(file, file_type, file_path, procyon_data)
        write_material_info(file, file_type, file_path, procyon_data)

        # subskeleton infos
        for bone_group in procyon_data.bone_groups:
            write_uint8(file, len(bone_group))
            for b in range(8):
                bone_index = bone_group[b] if b < len(bone_group) else UINT8_MAX
                write_uint8(file, bone_index)
            for a in range(3): write_uint8(file, 0) # alignment

        write_animation_info(file, file_type, procyon_data)
        write_mesh_data(file, file_type, procyon_data)
        write_bone_parent_indices(file, file_type, procyon_data)
        write_inverse_model_space_matrix(file, file_type, procyon_data)
        write_animation_data(file, file_type, procyon_data)
    file.close()

    write_diffuse_textures(file_path, procyon_data)

    return True

def write_static_info_header(file, file_type, procyon_data):
    if file_type == "P3D":
        write_string(file, " P3D") # 4 bytes (4)
        write_float(file, procyon_data.scale) # 4 bytes (8)
        write_uint32(file, procyon_data.num_vertex) # 4 bytes (12)
        write_uint32(file, procyon_data.num_index) # 4 bytes (16)
        write_uint16(file, len(procyon_data.meshes)) # 2 bytes (18)
        write_uint16(file, len(procyon_data.animations)) # 2 bytes (20)
        num_frames_total = sum([len(animation.frames) for animation in procyon_data.animations])
        write_uint16(file, num_frames_total)
        write_uint8(file, len(procyon_data.joints)) # 1 byte (21)
        write_uint8(file, 0) # alignment (24)
    elif file_type == "PP3D":
        write_string(file, "PP3D") # 4 bytes (4)
        write_float(file, procyon_data.scale) # 4 bytes (8)
        write_uint16(file, len(procyon_data.meshes)) # 2 bytes (10)
        write_uint16(file, len(procyon_data.materials)) # 2 bytes (12)
        write_uint16(file, len(procyon_data.bone_groups)) # 2 bytes (14)
        write_uint16(file, len(procyon_data.animations)) # 2 bytes (16)
        write_uint16(file, len(procyon_data.joints)) # 2 bytes (18)
        num_frames_total = sum([len(animation.frames) for animation in procyon_data.animations])
        write_uint16(file, num_frames_total) # 2 bytes (20)

def write_mesh_info(file, file_type, file_path, procyon_data):
    if file_type == "P3D":
        for mesh in procyon_data.meshes:
            write_uint32(file, len(mesh.indices))
            write_uint32(file, mesh.index_offset)
            write_uint32(file, mesh.vertex_offset)
            assert(mesh.material_index >= 0)
            diffuse_color = procyon_data.materials[mesh.material_index].diffuse_color
            write_uint32(file, color_to_uint32(diffuse_color))
            diffuse_image = procyon_data.materials[mesh.material_index].diffuse_image
            diffuse_image_name = f"{file_path.stem}_diffuse_{mesh.material_index}.png"
            diffuse_image_name_length = len(diffuse_image_name) if diffuse_image else 0
            assert(diffuse_image_name_length < 48)
            if diffuse_image:
                write_string(file, diffuse_image_name)
            for i in range(48 - diffuse_image_name_length):
                write_uint8(file, 0)
    elif file_type == "PP3D":
        for mesh in procyon_data.meshes:
            material_index = mesh.material_index if mesh.material_index >= 0 else UINT16_MAX
            write_uint16(file, material_index)
            subskeleton_index = mesh.subskeleton_index if mesh.subskeleton_index >= 0 else UINT16_MAX
            write_uint16(file, subskeleton_index)
            write_uint16(file, len(mesh.vertices))
            write_uint16(file, len(mesh.indices))

def write_animation_info(file, file_type, procyon_data):
    if file_type == "P3D":
        for animation in procyon_data.animations:
            assert(len(animation.name) < 64)
            write_string(file, animation.name)
            for i in range(64-len(animation.name)):
                write_uint8(file, 0)
            write_uint16(file, len(animation.frames))
    elif file_type == "PP3D":
        for animation in procyon_data.animations:
            assert(len(animation.name) < 64)
            write_string(file, animation.name)
            for i in range(64-len(animation.name)):
                write_uint8(file, 0)
            write_uint16(file, len(animation.frames))
            write_uint16(file, 0) # alignment

def write_mesh_data(file, file_type, procyon_data):
    print("Writing mesh data")
    if file_type == "P3D":
        # vertices
        for mesh in procyon_data.meshes:
            for vertex in mesh.vertices.keys():
                for e in range(0, 3):
                    vertex_position_element_int16 = float_to_int16(vertex.position[e] / procyon_data.scale)
                    write_int16(file, vertex_position_element_int16)
        for mesh in procyon_data.meshes:
            for vertex in mesh.vertices.keys():
                for e in range(0, 3):
                    vertex_normal_element_int16 = float_to_int16(vertex.normal[e])
                    write_int16(file, vertex_normal_element_int16)
        for mesh in procyon_data.meshes:
            for vertex in mesh.vertices.keys():
                for e in range(0, 2):
                    vertex_texcoord_element_int16 = float_to_int16(vertex.uv[e])
                    write_int16(file, vertex_texcoord_element_int16)
        for mesh in procyon_data.meshes:
            for vertex in mesh.vertices.keys():
                for e in range(0, 4):
                    if e < len(vertex.joint_indices):
                        write_uint8(file, vertex.joint_indices[e])
                    else:
                        write_uint8(file, 255)
        for mesh in procyon_data.meshes:
            for vertex in mesh.vertices.keys():
                for e in range(0, 4):
                    vertex_bone_weight_element_uint16 = 0
                    if e < len(vertex.joint_weights):
                        vertex_bone_weight_element_uint16 = float_to_uint16(vertex.joint_weights[e], a=0.0, b=1.0)
                    write_uint16(file, vertex_bone_weight_element_uint16)
        # indices
        for mesh in procyon_data.meshes:
            for index in mesh.indices:
                write_uint32(file, index)
    elif file_type == "PP3D":
        for mesh in procyon_data.meshes:
            for v, vertex in enumerate(mesh.vertices.keys()):
                for weight in vertex.joint_weights:
                    write_float(file, weight)
                for e in range(0, 2):
                    vertex_texcoord_element_int16 = float_to_int16(vertex.uv[e])
                    write_int16(file, vertex_texcoord_element_int16)
                for e in range(0, 3):
                    vertex_normal_element_int16 = float_to_int16(vertex.normal[e])
                    write_int16(file, vertex_normal_element_int16)
                for e in range(0, 3):
                    vertex_position_element_int16 = float_to_int16(vertex.position[e] / procyon_data.scale)
                    write_int16(file, vertex_position_element_int16)
            for i, index in enumerate(mesh.indices):
                write_uint16(file, index)

def write_material_info(file, file_type, file_path, procyon_data):
    if file_type == "P3D":
        pass
    elif file_type == "PP3D":
        for m, material in enumerate(procyon_data.materials):
            write_uint32(file, color_to_uint32(material.diffuse_color))
            diffuse_image = material.diffuse_image
            diffuse_image_name = f"{file_path.stem}_diffuse_{m}.png"
            diffuse_image_name_length = len(diffuse_image_name) if diffuse_image else 0
            assert(diffuse_image_name_length < 48)
            if diffuse_image:
                write_string(file, diffuse_image_name)
            for i in range(48 - diffuse_image_name_length):
                write_uint8(file, 0)

def write_bone_parent_indices(file, file_type, procyon_data):
    if file_type == "P3D":
        for joint in procyon_data.joints:
            bone_parent_index_uint8 = joint.parent_index if joint.parent_index >= 0 else UINT8_MAX
            write_uint8(file, bone_parent_index_uint8)
    elif file_type == "PP3D":
        for joint in procyon_data.joints:
            bone_parent_index_uint16 = joint.parent_index if joint.parent_index >= 0 else UINT16_MAX
            write_uint16(file, bone_parent_index_uint16)

def write_inverse_model_space_matrix(file, file_type, procyon_data):
    for joint in procyon_data.joints:
        for vector in joint.inverse_model_space_pose.transposed():
            for element in vector:
                write_float(file, element)

def write_animation_data(file, file_type, procyon_data):
    for animation in procyon_data.animations:
        for frame in animation.frames:
            for joint in frame.joints:
                write_float(file, joint.position.x)
                write_float(file, joint.position.y)
                write_float(file, joint.position.z)
                write_float(file, joint.rotation.x)
                write_float(file, joint.rotation.y)
                write_float(file, joint.rotation.z)
                write_float(file, joint.rotation.w)
                write_float(file, joint.scale.x)
                write_float(file, joint.scale.y)
                write_float(file, joint.scale.z)

def write_diffuse_textures(file_path, procyon_data):
    for material_index, material in enumerate(procyon_data.materials):
        if not material.diffuse_image: continue
        diffuse_texture_file_name = file_path.stem + f"_diffuse_{material_index}"
        diffuse_texture_file_path = file_path.with_name(diffuse_texture_file_name).with_suffix(".png")
        print(f"Writing diffuse texture: {diffuse_texture_file_path}")
        material.diffuse_image.save_render(str(diffuse_texture_file_path))



axes_enum = [("X","X","",1),("-X","-X","",2),("Y","Y","",3),("-Y","-Y","",4),("Z","Z","",5),("-Z","-Z","",6)]

class ExportProperties(bpy.types.PropertyGroup):
    # use_colors: bpy.props.BoolProperty(name="Use Colors", description="Include Vertex Colors", default=False)
    use_materials: bpy.props.BoolProperty(name="Use Materials", description="Include Mesh Materials", default=True)
    use_indices: bpy.props.BoolProperty(name="Use Indices (P3D)", description="Include Indices in P3D file", default=True)
    use_ascii: bpy.props.BoolProperty(name="Use ASCII (Debug)", description="Export data as plain text for debugging purposes", default=False)

    forward_axis: bpy.props.EnumProperty(name="", items=axes_enum, default="-Y")
    up_axis: bpy.props.EnumProperty(name="", items=axes_enum, default="Z")

    standard_path: bpy.props.StringProperty(name="P3D", description="Path for standard (P3D) file", subtype='FILE_PATH')
    portable_path: bpy.props.StringProperty(name="PP3D", description="Path for portable (PP3D) file", subtype='FILE_PATH')


class ExportStandardOperator(bpy.types.Operator):
    bl_idname = "object.export_standard"
    bl_label = "Export"

    def execute(self, context):
        write_proc(self, context, "P3D")
        return {'FINISHED'}

class ExportPortableOperator(bpy.types.Operator):
    bl_idname = "object.export_portable"
    bl_label = "Export"

    def execute(self, context):
        write_proc(self, context, "PP3D")
        return {'FINISHED'}


class ExportPanel(bpy.types.Panel):
    bl_label = "Export"
    bl_idname = "OBJECT_PT_layout"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "object"

    def split_columns(self, factor):
        new_split = self.layout.row().split(factor=factor)
        return new_split.column(), new_split.split().column()

    def draw(self, context):
        split_factor = 0.33

        properties_left_column, properties_right_column = self.split_columns(factor=split_factor)
        properties_left_column.label(text="Properties")
        # properties_right_column.prop(context.scene.export_properties, "use_colors")
        properties_right_column.prop(context.scene.export_properties, "use_materials")
        properties_right_column.prop(context.scene.export_properties, "use_indices")
        properties_right_column.prop(context.scene.export_properties, "use_ascii")

        axes_left_column, axes_right_column = self.split_columns(factor=split_factor)
        axes_left_column.label(text="Forward / Up")
        axes_right_column_row = axes_right_column.row()
        axes_right_column_row.prop(context.scene.export_properties, "forward_axis")
        axes_right_column_row.prop(context.scene.export_properties, "up_axis")

        standard_row = self.layout.row(align=False)
        standard_row.prop(context.scene.export_properties, "standard_path")
        standard_row.operator('object.export_standard')

        portable_row = self.layout.row(align=False)
        portable_row.prop(context.scene.export_properties, "portable_path")
        portable_row.operator('object.export_portable')



def register():
    bpy.utils.register_class(ExportProperties)
    bpy.utils.register_class(ExportStandardOperator)
    bpy.utils.register_class(ExportPortableOperator)
    bpy.utils.register_class(ExportPanel)
    bpy.types.Scene.export_properties = bpy.props.PointerProperty(type=ExportProperties)

def unregister():
    bpy.utils.unregister_class(ExportProperties)
    bpy.utils.unregister_class(ExportStandardOperator)
    bpy.utils.unregister_class(ExportPortableOperator)
    bpy.utils.unregister_class(ExportPanel)
    del bpy.types.Scene.export_properties

if __name__ == "__main__":
    register()
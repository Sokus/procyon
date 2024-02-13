import argparse
import sys
import struct
import mathutils
import pathlib
from copy import (copy)
import bpy
import bpy_extras
import bmesh

# for some reason `node_shader_utils` is not a module of `bpy_extras`,
# I cannot use `bpy_extras.node_shader_utils.rincipledBSDFWrapper`...
from bpy_extras.node_shader_utils import (PrincipledBSDFWrapper)

#--------------------------------------------------------------------------------
# Defines and classes
#--------------------------------------------------------------------------------

p_parsed_arguments = None # set by `p_argument_parser`

INT8_MIN = -128
INT8_MAX = 127
INT16_MAX = 32767
INT16_MIN = -32768

UINT8_MAX = 255
UINT16_MAX = 65535

class ProcyonVertex:
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

class ProcyonMaterial:
    def __init__(self):
        self.diffuse_color = []
        self.diffuse_image = None

class ProcyonMesh:
    def __init__(self):
        self.material_index = -1
        self.subskeleton_index = -1
        self.vertices = {}
        self.indices = []
        self.index_offset = 0
        self.vertex_offset = 0

class ProcyonSkeletonJoint:
    def __init__(self, name, parent_index, inverse_model_space_pose):
        self.name = name
        self.parent_index = parent_index
        self.inverse_model_space_pose = inverse_model_space_pose

class ProcyonAnimationJoint:
    def __init__(self, position, rotation, scale):
        self.position = position
        self.rotation = rotation
        self.scale = scale

class ProcyonAnimationFrame:
    def __init__(self):
        self.joints = []

class ProcyonAnimation:
    def __init__(self, name):
        self.name = name
        self.frames = []

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

class ProcyonArmatureObjectState:
    def __init__(self, pose_position, action, frame):
        self.pose_position = pose_position
        self.action = action
        self.frame = frame

#--------------------------------------------------------------------------------
# Utility functions
#--------------------------------------------------------------------------------

def p_float_to_int8(value, a=-1.0, b=1.0):
    float_negative_one_to_one = 2.0 * ((value - a) / (b - a)) - 1.0
    if float_negative_one_to_one >= 0:
        return int(float_negative_one_to_one * float(INT8_MAX))
    else:
        return int(float_negative_one_to_one * float(abs(INT8_MIN)))

def p_float_to_int16(value, a=-1.0, b=1.0):
    float_negative_one_to_one = 2.0 * ((value - a) / (b - a)) - 1.0
    if float_negative_one_to_one >= 0:
        return int(float_negative_one_to_one * float(INT16_MAX))
    else:
        return int(float_negative_one_to_one * float(abs(INT16_MIN)))

def p_float_to_uint16(value, a=-1.0, b=1.0):
    float_zero_to_one = (value - a) / (b - a)
    return int(float_zero_to_one * float(UINT16_MAX))

def p_color_to_uint32(value):
    r = int(value[0] * 255)
    g = int(value[1] * 255)
    b = int(value[2] * 255)
    a = int(value[3] * 255)
    return r | g << 8 | b << 16 | a << 24

def p_list_join_unique(list_a, list_b):
    new_list = list_a.copy()
    new_list.extend(e for e in list_b if e not in new_list)
    return new_list

#--------------------------------------------------------------------------------
# Get and process data
#--------------------------------------------------------------------------------

def p_get_axis_mapping_matrix():
    forward_axis = p_parsed_arguments.forward
    up_axis = p_parsed_arguments.up
    return bpy_extras.io_utils.axis_conversion("-Y", "Z", forward_axis, up_axis).to_4x4()

def p_get_visible_mesh_objects():
    mesh_objects = []
    for visible_object in bpy.context.visible_objects:
        if visible_object.type == "MESH":
            mesh_objects.append(visible_object)
    return mesh_objects

def p_get_visible_armature_objects():
    armature_objects = []
    for visible_object in bpy.context.visible_objects:
        if visible_object.type == "ARMATURE":
            armature_objects.append(visible_object)
    return armature_objects

def p_armature_object_get_state(armature_object):
    armature_object_state = ProcyonArmatureObjectState(
        pose_position = armature_object.data.pose_position,
        action = armature_object.animation_data.action,
        frame = bpy.context.scene.frame_current
    )
    return armature_object_state

def p_armature_object_set_state(armature_object, pose_position, action=None, frame=None):
    armature_object.data.pose_position = pose_position
    if action:
        armature_object.animation_data.action = action
    armature_object.data.update_tag()
    new_frame = frame if frame else bpy.context.scene.frame_current
    bpy.context.scene.frame_set(new_frame)

def p_joint_weights_normalized(weights):
    total_weight = sum(weights)
    result = []
    if total_weight != 0:
        for weight in weights:
            result.append(weight / total_weight)
    return result

def p_bmesh_triangulated(mesh):
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(mesh)

def p_procyon_data_process_mesh_objects(procyon_data, mesh_objects, armature):
    print("Procyon: Processing mesh objects...")
    if len(mesh_objects) > 0:
        transform_matrix = p_get_axis_mapping_matrix()
        scene_with_applied_modifiers = bpy.context.evaluated_depsgraph_get()
        p_materials = {}
        p_meshes = []

        dummy_color = [1.0, 1.0, 1.0, 1.0]
        dummy_material = bpy.data.materials.new("Dummy")
        dummy_material.diffuse_color = dummy_color

        for mesh_object in mesh_objects:
            mesh = mesh_object.evaluated_get(scene_with_applied_modifiers).to_mesh(preserve_all_data_layers=True, depsgraph=bpy.context.evaluated_depsgraph_get())
            mesh.transform(transform_matrix @ mesh_object.matrix_world)
            p_bmesh_triangulated(mesh)
            mesh.calc_normals_split()

            for polygon in mesh.polygons:
                assert(len(polygon.loop_indices) == 3) # mesh should've already been triangulated

                use_materials = not p_parsed_arguments.no_materials
                if use_materials and len(mesh.materials) > 0:
                    material = mesh.materials[polygon.material_index]
                else:
                    material = dummy_material  # TODO: check when is this branch reached

                material_index = 0
                if p_materials.get(material) == None:
                    material_bsdf = PrincipledBSDFWrapper(material)
                    if material_bsdf == None:
                        print(f"Procyon: Material '{material.name}, does not use PrincipledBSDF surface, aborting.")
                        return False

                    p_materials[material] = ProcyonMaterial()

                    if material_bsdf.base_color and len(material_bsdf.base_color) > 3:
                        alpha = material_bsdf.base_color[3]
                    else:
                        alpha = material_bsdf.alpha
                    p_materials[material].diffuse_color = [material_bsdf.base_color[0], material_bsdf.base_color[1], material_bsdf.base_color[2], alpha]

                    if use_materials:
                        base_color_socket = material_bsdf.node_principled_bsdf.inputs['Base Color']
                        p_materials[material].diffuse_image = base_color_socket.links[0].from_node.image
                        if len(base_color_socket.links) > 1:
                            print(f"WARNING: base color socket has {len(base_color_socket.links)} links, only the first one is processed!")
                material_index = list(p_materials.keys()).index(material)

                mesh_index = -1
                for index, p_mesh in enumerate(p_meshes):
                    if p_mesh.material_index != -1 and p_mesh.material_index == material_index:
                        mesh_index = index
                        break

                if not mesh_index >= 0:
                    mesh_index = len(p_meshes)
                    p_meshes.append(ProcyonMesh())
                    p_meshes[mesh_index].material_index = material_index

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
                                bone_name = mesh_object.vertex_groups[group.group].name
                                joint_indices.append(armature.data.bones.find(bone_name))
                                joint_weights.append(group.weight)

                    uv = mesh.uv_layers.active.data[loop.index].uv

                    vertex = ProcyonVertex(
                        mesh.vertices[loop.vertex_index].undeformed_co.copy(), # position
                        mathutils.Vector((uv.x, 1.0-uv.y)), # uv
                        loop.normal.copy(), # normal
                        dummy_color, # color
                        joint_indices,
                        p_joint_weights_normalized(joint_weights)
                    )

                    if p_meshes[mesh_index].vertices.get(vertex) == None:
                        p_meshes[mesh_index].vertices[vertex] = len(p_meshes[mesh_index].vertices)
                    p_meshes[mesh_index].indices.append(p_meshes[mesh_index].vertices[vertex])

        min_vector = [sys.float_info.max, sys.float_info.max, sys.float_info.max]
        max_vector = [sys.float_info.min, sys.float_info.min, sys.float_info.min]
        for mesh in p_meshes:
            for vertex in mesh.vertices.keys():
                for e in range(0, 3):
                    if vertex.position[e] < min_vector[e]:
                        min_vector[e] = vertex.position[e]
                    if vertex.position[e] > max_vector[e]:
                        max_vector[e] = vertex.position[e]
        procyon_data.scale = max(abs(min(min_vector)), abs(max(max_vector)))

        index_offset = 0
        vertex_offset = 0
        for mesh in p_meshes:
            procyon_data.num_vertex += len(mesh.vertices)
            procyon_data.num_index += len(mesh.indices)
            mesh.index_offset = index_offset
            mesh.vertex_offset = vertex_offset
            index_offset += len(mesh.indices)
            vertex_offset += len(mesh.vertices)

        procyon_data.materials = list(p_materials.values())
        procyon_data.meshes = p_meshes

def p_procyon_data_make_subskeletons(procyon_data):
    print("Procyon: Splitting meshes...")
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
    procyon_data.bone_groups = sorted(bone_connections, key=lambda x: x[:])

    new_meshes = []
    for mesh in procyon_data.meshes:
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

            subskeleton_index = None
            for group_index, group in enumerate(procyon_data.bone_groups):
                if all(bone_index in group for bone_index in triangle_bone_indices):
                    subskeleton_index = group_index

            assert(subskeleton_index) # at this point it should always be possible to find a suitable skeleton

            mesh_index = -1
            for m, new_mesh in enumerate(new_meshes):
                if new_mesh.material_index == mesh.material_index and new_mesh.subskeleton_index == subskeleton_index:
                    mesh_index = m
                    break
            if mesh_index < 0:
                mesh_index = len(new_meshes)
                new_meshes.append(ProcyonMesh())
                new_meshes[mesh_index].material_index = mesh.material_index
                new_meshes[mesh_index].subskeleton_index = subskeleton_index
            assert(mesh_index >= 0)

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


def p_procyon_data_make_subskeletons_old(procyon_data):
    print("Procyon: Making subskeletons...")
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
                new_bin = p_list_join_unique(bin, group)
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
            print("Procyon: Could not find a bone group with size equal or less than 8, aborting.", file=sys.stderr)
            return False

        procyon_data.bone_groups[best_candidate_bin_index] = sorted(p_list_join_unique(procyon_data.bone_groups[best_candidate_bin_index], bone_connections[best_candidate_index]))
        bone_connections.pop(best_candidate_index)
    procyon_data.bone_groups = sorted(procyon_data.bone_groups, key=lambda x: x[:])

    new_meshes = []
    for mesh in procyon_data.meshes:
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
                new_meshes.append(ProcyonMesh())
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

def p_procyon_data_process_armature_object(procyon_data, armature):
    if armature:
        print("Procyon: Processing armature object...")
        transform_matrix = p_get_axis_mapping_matrix()
        p_armature_object_set_state(armature, pose_position="POSE")

        for bone in armature.data.bones:
            parent_index = armature.data.bones.find(bone.parent.name) if bone.parent else -1
            inverse_model_space_pose = (transform_matrix @ bone.matrix_local).inverted()
            skeleton_joint = ProcyonSkeletonJoint(bone.name, parent_index,  inverse_model_space_pose)
            procyon_data.joints.append(skeleton_joint)

        for action in bpy.data.actions:
            # Ignore animations with name ending with .001, .002 etc
            if len(action.name) >= 4 and action.name[-4] == '.' and action.name[-3:].isdigit():
                continue
            p_animation = ProcyonAnimation(action.name)
            armature.animation_data.action = action
            start_frame = int(action.frame_range.x)
            end_frame = int(action.frame_range.y)
            for frame in range(start_frame, end_frame+1):
                p_frame = ProcyonAnimationFrame()
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
                    animation_joint = ProcyonAnimationJoint(translation, rotation, scale)
                    p_frame.joints.append(animation_joint)
                p_animation.frames.append(p_frame)
            procyon_data.animations.append(p_animation)

def p_procyon_data_collect():
    print("Procyon: Collecting data...")
    procyon_data = ProcyonData()

    selected_armature_objects = p_get_visible_armature_objects()
    armature = None
    if len(selected_armature_objects) == 1:
        armature = selected_armature_objects[0]
    elif len(selected_armature_objects) > 1:
        print("Procyon: Multiple armature objects selected, aborting.", file=sys.stderr)
        return None

    if armature:
        armature_object_state = p_armature_object_get_state(armature)
        p_armature_object_set_state(armature, pose_position="REST")


    mesh_objects = p_get_visible_mesh_objects()
    p_procyon_data_process_mesh_objects(procyon_data, mesh_objects, armature)

    p_procyon_data_process_armature_object(procyon_data, armature)

    if armature:
        p_armature_object_set_state(
            armature,
            pose_position = armature_object_state.pose_position,
            action = armature_object_state.action,
            frame = armature_object_state.frame
        )

        if p_parsed_arguments.portable:
            p_procyon_data_make_subskeletons_old(procyon_data)
            #p_procyon_data_make_subskeletons(procyon_data)

    return procyon_data

#--------------------------------------------------------------------------------
# Write output
#--------------------------------------------------------------------------------

def p_write_int32(file, value):
    use_ascii = p_parsed_arguments.ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("i", value))

def p_write_uint32(file, value):
    use_ascii = p_parsed_arguments.ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("I", value))

def p_write_int16(file, value):
    use_ascii = p_parsed_arguments.ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("h", value))

def p_write_uint16(file, value):
    use_ascii = p_parsed_arguments.ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("H", value))

def p_write_int8(file, value):
    use_ascii = p_parsed_arguments.ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("b", value))

def p_write_uint8(file, value):
    use_ascii = p_parsed_arguments.ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("B", value))

def p_write_float(file, value):
    use_ascii = p_parsed_arguments.ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("f", value))

def p_write_bool(file, value):
    use_ascii = p_parsed_arguments.ascii
    if use_ascii: file.write(str(value) + ' ')
    else: file.write(struct.pack("?", value))

def p_write_string(file, text):
    use_ascii = p_parsed_arguments.ascii
    if use_ascii: file.write(text)
    else: file.write(text.encode('ascii'))

def p3d_write(file, file_path, procyon_data):
    p3d_write_static_info_header(file, procyon_data)
    p3d_write_mesh_info(file, file_path, procyon_data)
    p3d_write_animation_info(file, procyon_data)
    p3d_write_mesh_data(file, procyon_data)
    p3d_write_bone_parent_indices(file, procyon_data)
    p_write_inverse_model_space_matrix(file, procyon_data)
    p_write_animation_data(file, procyon_data)

def p3d_write_static_info_header(file, procyon_data):
    print("Procyon: Writing static info header...")
    p_write_string(file, " P3D") # 4 bytes (4)
    p_write_float(file, procyon_data.scale) # 4 bytes (8)
    p_write_uint32(file, procyon_data.num_vertex) # 4 bytes (12)
    p_write_uint32(file, procyon_data.num_index) # 4 bytes (16)
    p_write_uint16(file, len(procyon_data.meshes)) # 2 bytes (18)
    p_write_uint16(file, len(procyon_data.animations)) # 2 bytes (20)
    num_frames_total = sum([len(animation.frames) for animation in procyon_data.animations])
    p_write_uint16(file, num_frames_total)
    p_write_uint8(file, len(procyon_data.joints)) # 1 byte (21)
    p_write_uint8(file, 0) # alignment (24)

def p3d_write_mesh_info(file, file_path, procyon_data):
    if len(procyon_data.meshes): print("Procyon: Writing mesh info...")
    for mesh in procyon_data.meshes:
        p_write_uint32(file, len(mesh.indices))
        p_write_uint32(file, mesh.index_offset)
        p_write_uint32(file, mesh.vertex_offset)
        assert(mesh.material_index >= 0)
        diffuse_color = procyon_data.materials[mesh.material_index].diffuse_color
        p_write_uint32(file, p_color_to_uint32(diffuse_color))
        diffuse_image = procyon_data.materials[mesh.material_index].diffuse_image
        diffuse_image_name = f"{file_path.stem}_diffuse_{mesh.material_index}.png"
        diffuse_image_name_length = len(diffuse_image_name) if diffuse_image else 0
        assert(diffuse_image_name_length < 48)
        if diffuse_image:
            p_write_string(file, diffuse_image_name)
        for i in range(48 - diffuse_image_name_length):
            p_write_uint8(file, 0)

def p3d_write_animation_info(file, procyon_data):
    if len(procyon_data.animations): print("Procyon: Writing animation info...")
    for animation in procyon_data.animations:
        assert(len(animation.name) < 64)
        p_write_string(file, animation.name)
        for i in range(64-len(animation.name)):
            p_write_uint8(file, 0)
        p_write_uint16(file, len(animation.frames))

def p3d_write_mesh_data(file, procyon_data):
    if len(procyon_data.meshes): print("Procyon: Writing mesh data...")
    # vertices
    for mesh in procyon_data.meshes:
        for vertex in mesh.vertices.keys():
            for e in range(0, 3):
                vertex_position_element_int16 = p_float_to_int16(vertex.position[e] / procyon_data.scale)
                p_write_int16(file, vertex_position_element_int16)
    for mesh in procyon_data.meshes:
        for vertex in mesh.vertices.keys():
            for e in range(0, 3):
                vertex_normal_element_int16 = p_float_to_int16(vertex.normal[e])
                p_write_int16(file, vertex_normal_element_int16)
    for mesh in procyon_data.meshes:
        for vertex in mesh.vertices.keys():
            for e in range(0, 2):
                vertex_texcoord_element_int16 = p_float_to_int16(vertex.uv[e])
                p_write_int16(file, vertex_texcoord_element_int16)
    for mesh in procyon_data.meshes:
        for vertex in mesh.vertices.keys():
            for e in range(0, 4):
                if e < len(vertex.joint_indices):
                    p_write_uint8(file, vertex.joint_indices[e])
                else:
                    p_write_uint8(file, 255)
    for mesh in procyon_data.meshes:
        for vertex in mesh.vertices.keys():
            for e in range(0, 4):
                vertex_bone_weight_element_uint16 = 0
                if e < len(vertex.joint_weights):
                    vertex_bone_weight_element_uint16 = p_float_to_uint16(vertex.joint_weights[e], a=0.0, b=1.0)
                p_write_uint16(file, vertex_bone_weight_element_uint16)
    # indices
    for mesh in procyon_data.meshes:
        for index in mesh.indices:
            p_write_uint32(file, index)

def p3d_write_bone_parent_indices(file, procyon_data):
    if len(procyon_data.joints): print("Procyon: Writing bone parent indices...")
    for joint in procyon_data.joints:
        bone_parent_index_uint8 = joint.parent_index if joint.parent_index >= 0 else UINT8_MAX
        p_write_uint8(file, bone_parent_index_uint8)

def pp3d_write(file, file_path, procyon_data):
    pp3d_write_static_info_header(file, procyon_data)
    pp3d_write_mesh_info(file, file_path, procyon_data)
    pp3d_write_material_info(file, file_path, procyon_data)
    pp3d_write_subskeleton_info(file, procyon_data)
    pp3d_write_animation_info(file, procyon_data)
    pp3d_write_mesh_data(file, procyon_data)
    pp3d_write_bone_parent_indices(file, procyon_data)
    p_write_inverse_model_space_matrix(file, procyon_data)
    p_write_animation_data(file, procyon_data)

def pp3d_write_static_info_header(file, procyon_data):
    print("Procyon: Writing static info header...")
    p_write_string(file, "PP3D") # 4 bytes (4)
    p_write_float(file, procyon_data.scale) # 4 bytes (8)
    p_write_uint16(file, len(procyon_data.meshes)) # 2 bytes (10)
    p_write_uint16(file, len(procyon_data.materials)) # 2 bytes (12)
    p_write_uint16(file, len(procyon_data.bone_groups)) # 2 bytes (14)
    p_write_uint16(file, len(procyon_data.animations)) # 2 bytes (16)
    p_write_uint16(file, len(procyon_data.joints)) # 2 bytes (18)
    num_frames_total = sum([len(animation.frames) for animation in procyon_data.animations])
    p_write_uint16(file, num_frames_total) # 2 bytes (20)

    p_write_int8(file, p_parsed_arguments.bone_weight_size) # 1 byte
    for a in range(3): p_write_uint8(file, 0) # alignment

def pp3d_write_mesh_info(file, file_path, procyon_data):
    if len(procyon_data.meshes): print("Procyon: Writing mesh info...")
    for mesh in procyon_data.meshes:
        material_index = mesh.material_index if mesh.material_index >= 0 else UINT16_MAX
        p_write_uint16(file, material_index)
        subskeleton_index = mesh.subskeleton_index if mesh.subskeleton_index >= 0 else UINT16_MAX
        p_write_uint16(file, subskeleton_index)
        p_write_uint16(file, len(mesh.vertices))
        p_write_uint16(file, len(mesh.indices))

def pp3d_write_material_info(file, file_path, procyon_data):
    if len(procyon_data.materials): print("Procyon: Writing material info...")
    for m, material in enumerate(procyon_data.materials):
        p_write_uint32(file, p_color_to_uint32(material.diffuse_color))
        diffuse_image = material.diffuse_image
        diffuse_image_name = f"{file_path.stem}_diffuse_{m}.png"
        diffuse_image_name_length = len(diffuse_image_name) if diffuse_image else 0
        assert(diffuse_image_name_length < 48)
        if diffuse_image:
            p_write_string(file, diffuse_image_name)
        for i in range(48 - diffuse_image_name_length):
            p_write_uint8(file, 0)

def pp3d_write_subskeleton_info(file, procyon_data):
    if len(procyon_data.bone_groups): print("Procyon: Writing subskeleton info...")
    for bone_group in procyon_data.bone_groups:
        p_write_uint8(file, len(bone_group))
        for b in range(8):
            bone_index = bone_group[b] if b < len(bone_group) else UINT8_MAX
            p_write_uint8(file, bone_index)
        for a in range(3): p_write_uint8(file, 0) # alignment

def pp3d_write_animation_info(file, procyon_data):
    if len(procyon_data.animations): print("Procyon: Writing animation info...")
    for animation in procyon_data.animations:
        assert(len(animation.name) < 64)
        p_write_string(file, animation.name)
        for i in range(64-len(animation.name)):
            p_write_uint8(file, 0)
        p_write_uint16(file, len(animation.frames))
        p_write_uint16(file, 0) # alignment

def pp3d_write_mesh_data(file, procyon_data):
    if len(procyon_data.meshes): print("Procyon: Writing mesh data...")
    for mesh in procyon_data.meshes:
        material = procyon_data.materials[mesh.material_index]
        for v, vertex in enumerate(mesh.vertices.keys()):
            pp3d_write_vertex_bone_weights(file, vertex)
            if material.diffuse_image:
                for e in range(0, 2):
                    vertex_texcoord_element_int16 = p_float_to_int16(vertex.uv[e])
                    p_write_int16(file, vertex_texcoord_element_int16)
            for e in range(0, 3):
                vertex_normal_element_int16 = p_float_to_int16(vertex.normal[e])
                p_write_int16(file, vertex_normal_element_int16)
            for e in range(0, 3):
                vertex_position_element_int16 = p_float_to_int16(vertex.position[e] / procyon_data.scale)
                p_write_int16(file, vertex_position_element_int16)
        for i, index in enumerate(mesh.indices):
            p_write_uint16(file, index)

def pp3d_write_vertex_bone_weights(file, vertex):
    bone_weight_size = p_parsed_arguments.bone_weight_size
    for weight in vertex.joint_weights:
        if bone_weight_size == 1:
            weight_int8 = p_float_to_int8(weight)
            p_write_int8(file, weight_int8)
        elif bone_weight_size == 2:
            weight_int16 = p_float_to_int16(weight)
            p_write_int16(file, weight_int16)
        elif bone_weight_size == 4:
            p_write_float(file, weight)
    bone_weights_size = len(vertex.joint_weights) * bone_weight_size
    if bone_weights_size % 2:
        p_write_uint8(file, 0) # pad to 16 bits

# TODO: Convert 16-bit index to 8-bit and merge with `p3d_write_bone_parent_indices`
def pp3d_write_bone_parent_indices(file, procyon_data):
    if len(procyon_data.joints): print("Procyon: Writing bone parent indices...")
    for joint in procyon_data.joints:
        bone_parent_index_uint16 = joint.parent_index if joint.parent_index >= 0 else UINT16_MAX
        p_write_uint16(file, bone_parent_index_uint16)

def p_write_inverse_model_space_matrix(file, procyon_data):
    if len(procyon_data.joints): print("Procyon: Writing inverse model space matrices...")
    for joint in procyon_data.joints:
        for vector in joint.inverse_model_space_pose.transposed():
            for element in vector:
                p_write_float(file, element)

def p_write_animation_data(file, procyon_data):
    if len(procyon_data.animations): print("Procyon: Writing animation data...")
    for animation in procyon_data.animations:
        for frame in animation.frames:
            for joint in frame.joints:
                p_write_float(file, joint.position.x)
                p_write_float(file, joint.position.y)
                p_write_float(file, joint.position.z)
                p_write_float(file, joint.rotation.x)
                p_write_float(file, joint.rotation.y)
                p_write_float(file, joint.rotation.z)
                p_write_float(file, joint.rotation.w)
                p_write_float(file, joint.scale.x)
                p_write_float(file, joint.scale.y)
                p_write_float(file, joint.scale.z)

def p_write_diffuse_textures(file_path, procyon_data):
    for material_index, material in enumerate(procyon_data.materials):
        if not material.diffuse_image: continue
        diffuse_texture_file_name = file_path.stem + f"_diffuse_{material_index}"
        diffuse_texture_file_path = file_path.with_name(diffuse_texture_file_name).with_suffix(".png")
        print(f"Procyon: Writing diffuse texture '{diffuse_texture_file_path}'")
        material.diffuse_image.save_render(str(diffuse_texture_file_path))

def p_main():
    print(f"Procyon: Started with parameters: {p_parsed_arguments}")

    procyon_data = p_procyon_data_collect()

    if not p_parsed_arguments.output:
        print("Procyon: No output path provided, aborting.", file=sys.stderr)
        return False

    file_write_mode = "w" if p_parsed_arguments.ascii else "wb"
    file_path = pathlib.Path(bpy.path.abspath(p_parsed_arguments.output))
    file = open(str(file_path), file_write_mode)

    print(f"Procyon: Writing to: '{file_path}'")
    if not p_parsed_arguments.portable:
        p3d_write(file, file_path, procyon_data)
    else:
        pp3d_write(file, file_path, procyon_data)

    file.close()

    p_write_diffuse_textures(file_path, procyon_data)

    return True

if __name__ == "__main__":
    if '--' in sys.argv:
        index_of_double_dash = sys.argv.index('--')
        input_arguments = sys.argv[(index_of_double_dash+1):]
    else:
        input_arguments = []

    p_argument_parser = argparse.ArgumentParser()
    p_argument_parser.add_argument("--output", metavar="<file>", help="place output into <file>")
    p_argument_parser.add_argument("--portable", action='store_true', help="export in portable format")
    p_argument_parser.add_argument("--no-materials", action='store_true', help="don't export mesh materials")
    p_argument_parser.add_argument("--bone_weight_size", default=1, type=int, choices=[1, 2, 4])
    axis_choices = ["X", "-X", "Y", "-Y", "Z", "-Z"]
    p_argument_parser.add_argument("--forward", default="-Z", choices=axis_choices, metavar="<axis>")
    p_argument_parser.add_argument("--up", default="Y", choices=axis_choices, metavar="<axis>",)
    p_argument_parser.add_argument("--ascii", action='store_true', help="write output in plain text")
    p_parsed_arguments = p_argument_parser.parse_args(input_arguments)

    p_main()
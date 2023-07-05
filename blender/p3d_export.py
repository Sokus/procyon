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
from bpy.utils import (register_class, unregister_class)
from bpy.types import (Panel, PropertyGroup)
from bpy.props import (StringProperty, BoolProperty, IntProperty, FloatProperty, FloatVectorProperty, EnumProperty, PointerProperty)
from bpy_extras.io_utils import (axis_conversion)
from bpy_extras.node_shader_utils import (PrincipledBSDFWrapper)

# Set to "wb" to output binary, or "w" to output plain text
file_write_mode = "wb"

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
    if file_write_mode == "wb": file.write(struct.pack("i", value))
    else: file.write(str(value) + ' ')

def write_uint32(file, value):
    if file_write_mode == "wb": file.write(struct.pack("I", value))
    else: file.write(str(value) + ' ')

def write_int16(file, value):
    if file_write_mode == "wb": file.write(struct.pack("h", value))
    else: file.write(str(value) + ' ')

def write_uint16(file, value):
    if file_write_mode == "wb": file.write(struct.pack("H", value))
    else: file.write(str(value) + ' ')

def write_uint8(file, value):
    if file_write_mode == "wb": file.write(struct.pack("B", value))
    else: file.write(str(value) + ' ')

def write_float(file, value):
    if file_write_mode == "wb": file.write(struct.pack("f", value))
    else: file.write(str(value) + ' ')

def write_bool(file, value):
    if file_write_mode == "wb": file.write(struct.pack("?", value))
    else: file.write(str(value) + ' ')

def write_string(file, text):
    if file_write_mode == "wb": file.write(text.encode('ascii'))
    else: file.write(text)

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
    INT16_MAX = 32767
    return int(float_negative_one_to_one * float(INT16_MAX))

def float_to_uint16(value, a=-1.0, b=1.0):
    float_zero_to_one = (value - a) / (b - a)
    UINT16_MAX = 65535
    return int(float_zero_to_one * float(UINT16_MAX))

def color_to_uint32(value):
    r = int(value[0] * 255)
    g = int(value[1] * 255)
    b = int(value[2] * 255)
    a = int(value[3] * 255)
    return r | g << 8 | b << 16 | a << 24

def write_joints(file, armature, transform):
    for bone in armature.data.bones:
        write_uint8(file, armature.data.bones.find(bone.parent.name) if bone.parent else 0)
        model_space_pose = transform @ bone.matrix_local
        inverse_model_space_pose = model_space_pose.inverted()
        for vector in inverse_model_space_pose:
            for float in vector:
                write_float(file, float)

def write_animation(file, armature, animation, transform):
    start_frame = int(animation.frame_range.x)
    end_frame = int(animation.frame_range.y)
    armature.animation_data.action = animation

    write_uint32(file, end_frame-start_frame + 1)
    print(end_frame-start_frame + 1)
    write_uint32(file, len(animation.name))
    write_string(file, animation.name)
    for frame in range(start_frame, end_frame+1):
        bpy.context.scene.frame_set(frame)
        for bone in armature.pose.bones:
            parentSpacePose = bone.matrix
            if bone.parent:
                parentSpacePose = bone.parent.matrix.inverted() @ bone.matrix
            else:
                parentSpacePose = transform @ bone.matrix
            translation = parentSpacePose.to_translation()
            write_float(file, translation.x)
            write_float(file, translation.y)
            write_float(file, translation.z)
            rotation = parentSpacePose.to_quaternion()
            write_float(file, rotation.w)
            write_float(file, rotation.x)
            write_float(file, rotation.y)
            write_float(file, rotation.z)
            # Does not support negative scales
            scale = parentSpacePose.to_scale()
            write_float(file, scale.x)
            write_float(file, scale.y)
            write_float(file, scale.z)


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

def write_p3d(context, file, procyon_data):
    # Write header
    write_string(file, " P3D")
    write_float(file, procyon_data.scale)
    write_int32(file, procyon_data.num_vertex)
    write_int32(file, procyon_data.num_index)
    write_uint16(file, len(procyon_data.meshes))
    write_uint16(file, 0) # alignment

    # Write vertex data
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
                vertex_texcoord_element_uint16 = float_to_uint16(vertex.uv[e], a=0.0, b=1.0)
                write_uint16(file, vertex_texcoord_element_uint16)

    # Write index data
    for mesh in procyon_data.meshes:
        for index in mesh.indices:
            write_uint32(file, index)

    # Write meshes
    for mesh in procyon_data.meshes:
        write_uint32(file, len(mesh.indices))
        write_uint32(file, mesh.index_offset)
        write_uint32(file, mesh.vertex_offset)
        write_uint32(file, color_to_uint32(mesh.diffuse_color))
    return True

def write_pp3d(context, file, procyon_data):
    # Write header
    write_string(file, "PP3D") # 4 bytes
    write_float(file, procyon_data.scale) # 4 bytes
    write_uint16(file, len(procyon_data.meshes)) # 2 bytes
    write_uint16(file, 0) # alignment to 12 bytes

    # Write mesh info
    for mesh in procyon_data.meshes:
        write_uint16(file, len(mesh.vertices))
        write_uint16(file, len(mesh.indices))

        material_diffuse_color = procyon_data.materials[mesh.material_index].diffuse_color
        write_uint32(file, color_to_uint32(material_diffuse_color))

    # Write mesh vertices and indices
    for mesh in procyon_data.meshes:
        for vertex in mesh.vertices.keys():
            for e in range(0, 2):
                vertex_texcoord_element_int16 = float_to_int16(vertex.uv[e], a=0.0, b=1.0)
                write_int16(file, vertex_texcoord_element_int16)
            for e in range(0, 3):
                vertex_normal_element_int16 = float_to_int16(vertex.normal[e])
                write_int16(file, vertex_normal_element_int16)
            for e in range(0, 3):
                vertex_position_element_int16 = float_to_int16(vertex.position[e] / procyon_data.scale)
                write_int16(file, vertex_position_element_int16)
        for index in mesh.indices:
            write_uint16(file, index)

    return True

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
                        materials[material] = Material()
                        material_bsdf = PrincipledBSDFWrapper(material)
                        if material_bsdf:
                            if material_bsdf.base_color and len(material_bsdf.base_color) > 3:
                                alpha = material_bsdf.base_color[3]
                            else:
                                alpha = material_bsdf.alpha
                            materials[material].diffuse_color = [material_bsdf.base_color[0], material_bsdf.base_color[1], material_bsdf.base_color[2], alpha]
                        else:
                            operator.report({"ERROR"}, "Material '", material.name, "' does not use PrincipledBSDF surface, not parsing.")
                            return False
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
            model_space_pose = transform_matrix @ bone.matrix_local
            skeleton_joint = SkeletonJoint(bone.name, parent_index, model_space_pose.inverted())
            procyon_data.joints.append(skeleton_joint)


        for action in bpy.data.actions:
            # Ignore animations with name ending with .001, .002 etc
            if action.name[-4] == '.' and action.name[-3:].isdigit():
                continue
            p_animation = Animation(action.name)
            armature.animation_data.action = action
            start_frame = int(action.frame_range.x)
            end_frame = int(action.frame_range.y)
            for frame in range(start_frame, end_frame+1):
                p_frame = AnimationFrame()
                bpy.context.scene.frame_set(frame)
                for b, bone in enumerate(armature.pose.bones):
                    translation = mathutils.Vector((0.0, 0.0, 0.0))
                    rotation = mathutils.Quaternion()
                    scale = mathutils.Vector((1.0, 1.0, 1.0))
                    if file_type == "P3D":
                        # FIXME: This was not touched  yet
                        parent_space_pose = bone.matrix_channel
                        if bone.parent:
                            parent_space_pose = bone.parent.matrix.inverted() @ bone.matrix
                        else:
                            parent_space_pose = transform_matrix @ bone.matrix
                        translation = parent_space_pose.to_translation()
                        rotation = parent_space_pose.to_quaternion()
                        scale = transform_matrix @ armature.matrix_world @ parent_space_pose.to_scale() # Does not support negative values
                    elif file_type == "PP3D":
                        inverse_model_space_pose = armature.data.bones[b].matrix_local.inverted()
                        armature_space_pose = transform_matrix @ inverse_model_space_pose @ bone.matrix

                        translation_scale = (transform_matrix @ armature.matrix_world).to_scale()
                        translation = armature_space_pose.to_translation().cross(translation_scale)
                        rotation = armature_space_pose.to_quaternion()
                        scale = armature_space_pose.to_scale()

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

            #print("new meshes:")
            #for m in new_meshes: print(f"mat:{m.material_index} sk:{m.subskeleton_index} v:{len(m.vertices)} i:{len(m.indices)}")




    if file_type == "P3D":
        file = open(bpy.path.abspath(context.scene.export_properties.standard_path), file_write_mode)
        write_p3d(context, file, procyon_data)
    elif file_type == "PP3D":
        file = open(bpy.path.abspath(context.scene.export_properties.portable_path), file_write_mode)
        write_pp3d(context, file, procyon_data)
    file.close()

    return True


axes_enum = [("X","X","",1),("-X","-X","",2),("Y","Y","",3),("-Y","-Y","",4),("Z","Z","",5),("-Z","-Z","",6)]

class ExportProperties(bpy.types.PropertyGroup):
    # use_colors: bpy.props.BoolProperty(name="Use Colors", description="Include Vertex Colors", default=False)
    use_materials: bpy.props.BoolProperty(name="Use Materials", description="Include Mesh Materials", default=True)
    use_indices: bpy.props.BoolProperty(name="Use Indices (P3D)", description="Include Indices in P3D file", default=True)

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
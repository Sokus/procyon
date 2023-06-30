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

class Mesh:
    def __init__(self):
        # self.has_vertex_colors = False
        self.vertices = {}
        self.indices = []
        self.index_offset = 0
        self.vertex_offset = 0
        self.diffuse_color = []

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
        self.joints = []
        self.animations = []

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
    total_weights = sum(weights)
    result = [0,0,0,0]
    if total_weights != 0:
        for i, weight in enumerate(weights):
            result[i] = weight / total_weights
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
        write_uint32(file, color_to_uint32(mesh.diffuse_color))

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

def write_proc(context, file_type):
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
        meshes = {}

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
                    use_materials = bpy.context.scene.export_properties.use_materials == True
                    if  use_materials == True and len(mesh.materials) > 0:
                        material = mesh.materials[polygon.material_index]
                    else:
                        material = dummy_material

                    if meshes.get(material) == None:
                        meshes[material] = Mesh()
                        material_bsdf = PrincipledBSDFWrapper(material)
                        if material_bsdf:
                            if material_bsdf.base_color and len(material_bsdf.base_color) > 3:
                                alpha = material_bsdf.base_color[3]
                            else:
                                alpha = material_bsdf.alpha
                            meshes[material].diffuse_color = [material_bsdf.base_color[0], material_bsdf.base_color[1], material_bsdf.base_color[2], alpha]
                        else:
                            self.report({"ERROR"}, "Material '", material.name, "' does not use PrincipledBSDF surface, not parsing.")

                    # Get mesh vertices, indices and joint info
                    face_indices = []
                    for loop_index in polygon.loop_indices:
                        loop = mesh.loops[loop_index]

                        joint_indices = [0,0,0,0]
                        joint_weights = [0,0,0,0]
                        if armature:
                            for joint_binding_index, group in enumerate(mesh.vertices[loop.vertex_index].groups):
                                if joint_binding_index < 4:
                                    group_index = group.group
                                    bone_name = object.vertex_groups[group_index].name
                                    joint_indices[joint_binding_index] = armature.data.bones.find(bone_name)
                                    joint_weights[joint_binding_index] = group.weight

                        uv = mesh.uv_layers.active.data[loop.index].uv

                        vertex = Vertex(
                            mesh.vertices[loop.vertex_index].undeformed_co.copy(), # position
                            mathutils.Vector((uv.x, 1.0-uv.y)), # uv
                            loop.normal.copy(), # normal
                            dummy_color, # color
                            joint_indices,
                            normalize_joint_weights(joint_weights)
                        )

                        if meshes[material].vertices.get(vertex) == None:
                            meshes[material].vertices[vertex] = len(meshes[material].vertices)
                        meshes[material].indices.append(meshes[material].vertices[vertex])

        min_vector = [sys.float_info.max, sys.float_info.max, sys.float_info.max]
        max_vector = [sys.float_info.min, sys.float_info.min, sys.float_info.min]
        for mesh in meshes.values():
            for vertex in mesh.vertices.keys():
                for e in range(0, 3):
                    if vertex.position[e] < min_vector[e]:
                        min_vector[e] = vertex.position[e]
                    if vertex.position[e] > max_vector[e]:
                        max_vector[e] = vertex.position[e]
        procyon_data.scale = max(abs(min(min_vector)), abs(max(max_vector)))

        index_offset = 0
        vertex_offset = 0
        for mesh in meshes.values():
            procyon_data.num_vertex += len(mesh.vertices)
            procyon_data.num_index += len(mesh.indices)
            mesh.index_offset = index_offset
            mesh.vertex_offset = vertex_offset
            index_offset += len(mesh.indices)
            vertex_offset += len(mesh.vertices)

        procyon_data.meshes = meshes.values()

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
                for bone in armature.pose.bones:
                    parent_space_pose = bone.matrix
                    if bone.parent:
                        parent_space_pose = bone.parent.matrix.inverted() @ bone.matrix
                    else:
                        parent_space_pose = transform_matrix @ bone.matrix
                    translation = parent_space_pose.to_translation()
                    rotation = parent_space_pose.to_quaternion()
                    scale = parent_space_pose.to_scale() # Does not support negative values
                    animation_joint = AnimationJoint(translation, rotation, scale)
                    p_frame.joints.append(animation_joint)
                p_animation.frames.append(p_frame)
            procyon_data.animations.append(p_animation)
        # Change armature back to the pose it was in.
        bpy.context.scene.frame_set(original_frame)
        armature.animation_data.action = original_action
        set_armature_position(armature, original_armature_position)

    joint_groups = []

    for j in range(len(procyon_data.joints)):
        for mesh in procyon_data.meshes:
            for f in range(int(len(mesh.indices)/3)):
                # Find all joint indices that are referenced by a triangle
                face_joint_indices = []
                for index in mesh.indices[(3*f):(3*f+3)]:
                    vertex = list(mesh.vertices.keys())[index]
                    joint_indices = [vertex.joint_indices[idx] for idx, weight in enumerate(vertex.joint_weights) if weight > 0.0]
                    face_joint_indices.extend(x for x in joint_indices if x not in face_joint_indices)
                # Check if referenced joint indices already exist in any joint group.
                # Indices can be contained in a few groups at the same time, if that is the case join the groups together.
                # If joint indices were not found in any existing group create a new one.
                if len(face_joint_indices) > 0:
                    first_joint_group = None
                    for joint_group in joint_groups:
                        common_joints = [element for element in face_joint_indices if element in joint_group]
                        if len(common_joints) > 0:
                            if first_joint_group == None:
                                first_joint_group = joint_group
                                first_joint_group.extend(element for element in face_joint_indices if element not in first_joint_group)
                            else:
                                joint_group.clear()
                    if first_joint_group == None:
                        joint_groups.append(face_joint_indices)
                    joint_groups = [group for group in joint_groups if group]

    # Sort the groups by indices
    joint_groups = [sorted(group) for group in joint_groups]
    joint_groups.sort(key=lambda group: group[0])

    for group in joint_groups:
        names = [procyon_data.joints[index].name for index in group]
        print(names)
    groupless_joints = []
    for j, joint in enumerate(procyon_data.joints):
        groupless = True
        for group in joint_groups:
            if j in group:
                groupless = False
                break
        if groupless == True:
            groupless_joints.append(joint.name)
    print("Groupless:", groupless_joints)


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
        write_proc(context, "P3D")
        return {'FINISHED'}

class ExportPortableOperator(bpy.types.Operator):
    bl_idname = "object.export_portable"
    bl_label = "Export"

    def execute(self, context):
        write_proc(context, "PP3D")
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
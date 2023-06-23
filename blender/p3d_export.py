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
        self.vertices = {}
        self.indices = []
        self.index_offset = 0
        self.diffuse_color = []

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

def get_data_from_mesh_objects(objects, armature, transformMatrix):
    scene_with_applied_modifiers = bpy.context.evaluated_depsgraph_get()

    meshes = {}
    num_vertex = 0
    num_index = 0

    dummy_color = [0.8, 0.8, 0.8, 1.0]
    dummy_material = bpy.data.materials.new("Dummy")
    dummy_material.diffuse_color = dummy_color

    for object in objects:
        # Make a copy of the mesh with applied modifiers
        mesh = object.evaluated_get(scene_with_applied_modifiers).to_mesh(preserve_all_data_layers=True, depsgraph=bpy.context.evaluated_depsgraph_get())

        mesh.transform(transformMatrix @ object.matrix_world)
        triangulate_mesh(mesh)
        mesh.calc_normals_split()

        # Get vertex color palette
        if len(mesh.vertex_colors) > 0 and len(mesh.vertex_colors[mesh.vertex_colors.active_index].data) > 0:
            vertex_colors = mesh.vertex_colors[mesh.vertex_colors.active_index].data
        else:
            vertex_colors = []

        for polygon in mesh.polygons:
            if len(polygon.loop_indices) == 3:

                # Get mesh material
                if bpy.context.scene.export_properties.use_materials == True and len(mesh.materials) > 0:
                    material = mesh.materials[polygon.material_index]
                else:
                    material = dummy_material

                # Add mesh to dictionary and extract material properties
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
                    position = mesh.vertices[loop.vertex_index].undeformed_co
                    uv = mesh.uv_layers.active.data[loop.index].uv
                    uv.y = 1-uv.y
                    normal = loop.normal

                    if len(vertex_colors) > 0:
                        color = vertex_colors[loop.vertex_index].color
                    else:
                        color = dummy_color

                    joint_indices = [0,0,0,0]
                    joint_weights = [0,0,0,0]
                    if armature:
                        for joint_binding_index, group in enumerate(mesh.vertices[loop.vertex_index].groups):
                            if joint_binding_index < 4:
                                group_index = group.group
                                bone_name = object.vertex_groups[group_index].name
                                joint_indices[joint_binding_index] = armature.data.bones.find(bone_name)
                                joint_weights[joint_binding_index] = group.weight

                    vertex = Vertex(position, uv, normal, color, joint_indices, normalize_joint_weights(joint_weights))

                    meshes[material].indices.append(len(meshes[material].vertices))
                    if meshes[material].vertices.get(vertex) == None:
                        meshes[material].vertices[vertex] = len(meshes[material].vertices)

    min_vector = [sys.float_info.max, sys.float_info.max, sys.float_info.max]
    max_vector = [sys.float_info.min, sys.float_info.min, sys.float_info.min]
    for mesh in meshes.values():
        for vertex in mesh.vertices.keys():
            for e in range(0, 3):
                if vertex.position[e] < min_vector[e]:
                    min_vector[e] = vertex.position[e]
                if vertex.position[e] > max_vector[e]:
                    max_vector[e] = vertex.position[e]
    scale = max(abs(min(min_vector)), abs(max(max_vector)))

    index_offset = 0
    for mesh in meshes.values():
        num_vertex += len(mesh.vertices)
        num_index += len(mesh.indices)
        mesh.index_offset = index_offset
        index_offset += len(mesh.indices)

    return scale, num_vertex, num_index, meshes.values()

def float_to_int16(value):
    INT16_MAX = 32767
    return int(value * float(INT16_MAX))

def float_to_uint16(value):
    UINT16_MAX = 65535
    return int(value * float(UINT16_MAX))

def color_to_uint32(value):
    r = int(value[0] * 255)
    g = int(value[1] * 255)
    b = int(value[2] * 255)
    a = int(value[3] * 255)
    return r << 24 | g << 16 | b << 8 | a

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

axes_enum = [("X","X","",1),("-X","-X","",2),("Y","Y","",3),("-Y","-Y","",4),("Z","Z","",5),("-Z","-Z","",6)]

indices_enum = [
    ("always", "Always"  , "Always include indexed vertices"       , 1),
    ("p3d_only", "P3D Only", "Include indexed vertices in P3D format", 2),
    ("never", "Never"   , "Never include indexed vertices"        , 3)]

class ExportProperties(bpy.types.PropertyGroup):
    use_colors: bpy.props.BoolProperty(name="Use Colors", description="Include Vertex Colors", default=False)
    use_materials: bpy.props.BoolProperty(name="Use Materials", description="Include Mesh Materials", default=True)

    use_indices: bpy.props.EnumProperty(name="", items=indices_enum, default="always")

    forward_axis: bpy.props.EnumProperty(name="", items=axes_enum, default="-Y")
    up_axis: bpy.props.EnumProperty(name="", items=axes_enum, default="Z")

    standard_path: bpy.props.StringProperty(name="P3D", description="Path for standard (P3D) file", subtype='FILE_PATH')
    portable_path: bpy.props.StringProperty(name="PP3D", description="Path for portable (PP3D) file", subtype='FILE_PATH')


class ExportStandardOperator(bpy.types.Operator):
    bl_idname = "object.export_standard"
    bl_label = "Export"

    def execute(self, context):
        armature = get_selected_armature()

        # If the mesh has an armature modifier, the current pose will be applied to vertices, so change it to rest position
        original_armature_position = "REST"
        if armature:
            original_armature_position = armature.data.pose_position
            set_armature_position(armature, "REST")

        objects = get_selected_mesh_objects()
        if len(objects) > 0:
            scale, num_vertex, num_index, meshes = get_data_from_mesh_objects(objects, armature, get_axis_mapping_matrix())
            file = open(bpy.path.abspath(context.scene.export_properties.standard_path), file_write_mode)
            use_indices = context.scene.export_properties.use_indices in {"always", "p3d_only"}

            # Write header
            write_string(file, " P3D")
            write_float(file, scale)
            if use_indices:
                write_int32(file, num_vertex)
                write_int32(file, num_index)
            else:
                write_int32(file, num_index)
                write_int32(file, 0)
            write_uint16(file, len(meshes))

            # Write vertex data
            for mesh in meshes:
                if use_indices:
                    for vertex in mesh.vertices.keys():
                        for e in range(0, 3):
                            vertex_position_element_int16 = float_to_int16(vertex.position[e] / scale)
                            write_int16(file, vertex_position_element_int16)
                    for vertex in mesh.vertices.keys():
                        for e in range(0, 3):
                            vertex_normal_element_int16 = float_to_int16(vertex.normal[e])
                            write_int16(file, vertex_normal_element_int16)
                    for vertex in mesh.vertices.keys():
                        for e in range(0, 2):
                            vertex_texcoord_element_uint16 = float_to_uint16(vertex.uv[e])
                            write_uint16(file, vertex_texcoord_element_uint16)
                    for vertex in mesh.vertices.keys():
                        write_uint32(file, color_to_uint32(vertex.color))
                    for index in mesh.indices:
                        write_uint32(file, index)
                else:
                    vertices = list(mesh.vertices.keys())
                    for index in mesh.indices:
                        print(index,", ")
                        for e in range(0, 3):
                            vertex_position_element_int16 = float_to_int16(vertices[index].position[e] / scale)
                            write_int16(file, vertex_position_element_int16)
                    for index in mesh.indices:
                        for e in range(0, 3):
                            vertex_normal_element_int16 = float_to_int16(vertices[index].normal[e])
                            write_int16(file, vertex_normal_element_int16)
                    for index in mesh.indices:
                        for e in range(0, 2):
                            vertex_texcoord_element_uint16 = float_to_uint16(vertices[index].uv[e])
                            write_uint16(file, vertex_texcoord_element_uint16)
                    for index in mesh.indices:
                        write_uint32(file, color_to_uint32(vertices[index].color))

            # Write meshes
            for mesh in meshes:
                if use_indices:
                    write_uint32(file, len(mesh.indices))
                else:
                    write_uint32(file, len(mesh.vertices))
                write_uint32(file, mesh.index_offset)
                write_uint32(file, color_to_uint32(mesh.diffuse_color))

        # Change armature back to the pose it was in.
        if armature:
            set_armature_position(armature, original_armature_position)

        return {'FINISHED'}

class ExportPortableOperator(bpy.types.Operator):
    bl_idname = "object.export_portable"
    bl_label = "Export"

    def execute(self, context):
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
        properties_right_column.prop(context.scene.export_properties, "use_colors")
        properties_right_column.prop(context.scene.export_properties, "use_materials")

        indices_left_column, indices_right_column = self.split_columns(factor=split_factor)
        indices_left_column.label(text="Indexed")
        indices_right_column.prop(context.scene.export_properties, "use_indices")

        axes_left_column, axes_right_column = self.split_columns(factor=split_factor)
        axes_left_column.label(text="Forward / Up")
        axes_right_column_row = axes_right_column.row()
        axes_right_column_row.prop(context.scene.export_properties, "forward_axis")
        axes_right_column_row.prop(context.scene.export_properties, "up_axis")

        standard_row = self.layout.row(align=False)
        standard_row.prop(context.scene.export_properties, "standard_path")
        standard_row.operator('object.export_standard')

        standard_row = self.layout.row(align=False)
        standard_row.prop(context.scene.export_properties, "portable_path")
        standard_row.operator('object.export_portable')

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
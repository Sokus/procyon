bl_info = {
    "name": "P3D Export",
    "author": "Sokus",
    "version": (0, 0, 1),
    "blender": (3, 2, 0),
    "location": "File > Export",
    "description": "Procyon 3D (.p3d) format.",
    "category": "Export"}

import bmesh
import os
from struct import pack, unpack
from mathutils import Matrix
from bpy_extras import io_utils, node_shader_utils
from bpy_extras.wm_utils.progress_report import (
    ProgressReport,
    ProgressReportSubstep,
    )

def write_p3d(context, filepath, report, *,
        use_selection=False,
        use_uvs=True,
        use_colors=True
        ):

    # Get objects
    depsgraph = context.evaluated_depsgraph_get()
    scene = context.scene
    if use_selection:
        objects = context.selected_objects
    else:
        objects = context.scene.objects

    # Set rest armature
    oldaction = None
    oldframe = context.scene.frame_current
    oldpose = {}
    for i, ob_main in enumerate(objects):
        if ob_main.type == "ARMATURE":
            oldpose[i] = ob_main.data.pose_position
            ob_main.data.pose_position = "REST"
            ob_main.data.update_tag()
            if oldaction == None:
                oldaction = ob_main.animation_data.action
    context.scene.frame_set(0)



    return {'FINISHED'}


# -----------------------------------------------------------------------------
# Blender integration
import bpy
from bpy.props import (
        BoolProperty,
        FloatProperty,
        StringProperty,
        IntProperty,
        EnumProperty,
        )
from bpy_extras.io_utils import (
        ExportHelper,
        ImportHelper,
        axis_conversion,
        )

class ExportP3D(bpy.types.Operator, ExportHelper):
    """Save a Procyon 3D file (.p3d)"""

    bl_idname = "export_scene.p3d"
    bl_label = 'Export P3D'
    bl_options = {'PRESET'}

    filename_ext = ".p3d"
    filter_glob: StringProperty(
            default="*.p3d",
            options={'HIDDEN'},
            )

    # import range
    use_selection: BoolProperty(
            name="Selection Only",
            description="Export selected objects only",
            default=False,
            )
    # export properties
    use_uvs: BoolProperty(
            name="Include UVs",
            description="Write out the active UV coordinates",
            default=True,
            )
    use_colors: BoolProperty(
            name="Include Vertex Colors",
            description="Write out individual vertex colors (independent to material colors)",
            default=True,
            )



    def execute(self, context):
        # Exit edit mode before exporting, so current object states are exported properly.
        if bpy.ops.object.mode_set.poll():
            bpy.ops.object.mode_set(mode='OBJECT')

        keywords = self.as_keywords(ignore=("filepath", "filter_glob"))
        return write_p3d(context, self.filepath, self.report, **keywords)


def menu_func_export(self, context):
    self.layout.operator(ExportP3D.bl_idname, text="Procyon 3D (.p3d)")


def register():
    bpy.utils.register_class(ExportP3D)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)
#    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)


def unregister():
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)
#    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)
    bpy.utils.unregister_class(ExportP3D)


if __name__ == "__main__":
    register()

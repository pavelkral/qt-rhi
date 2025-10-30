# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## cubemap Binary:
#####################################################################

qt_internal_add_manual_test(cubemap
    GUI
    SOURCES
        cubemap.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)
set_source_files_properties("../shared/OpenfootageNET_fieldairport-512.hdr"
    PROPERTIES QT_RESOURCE_ALIAS "OpenfootageNET_fieldairport-512.hdr"
)
# Resources:
set(cubemap_resource_files
    "c.png"
    "cubemap.frag.qsb"
    "cubemap.vert.qsb"
    "equirect_to_cubemap.frag.qsb"
    "../shared/OpenfootageNET_fieldairport-512.hdr"
)

qt_internal_add_resource(cubemap "cubemap"
    PREFIX
        "/"
    FILES
        ${cubemap_resource_files}
)

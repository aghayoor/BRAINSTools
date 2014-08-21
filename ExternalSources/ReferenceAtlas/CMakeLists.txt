project(ReferenceAtlas)
cmake_minimum_required(VERSION 2.8)

## ReferenceAtlas_XML_DIR defined from calling environment

# Define the atlas subdirectory in one place
set(ATLAS_NAME Atlas/Atlas_${ATLAS_VERSION})
set(ATLAS_DIRECTORY ${ReferenceAtlas_XML_DIR}/${ATLAS_NAME})
set(ATLAS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

#
# In order to test need an XML file

## Configure the standard version
configure_file(${ATLAS_SOURCE_DIR}/ExtendedAtlasDefinition.xml.in ${ATLAS_DIRECTORY}/AtlasPVDefinition.xml @ONLY IMMEDIATE)
file(COPY ${ATLAS_SOURCE_DIR}/ExtendedAtlasDefinition.xml.in DESTINATION ${ATLAS_DIRECTORY}/ USE_SOURCE_PERMISSIONS)
install( FILES  ${ATLAS_DIRECTORY}/AtlasPVDefinition.xml DESTINATION bin/${ATLAS_NAME} )

file(GLOB AtlasImageFiles "${ATLAS_SOURCE_DIR}/*.nii.gz")
foreach(IMAGES ${AtlasImageFiles})
  get_filename_component(image ${IMAGES} NAME)
  file(COPY ${IMAGES} DESTINATION ${ATLAS_DIRECTORY}/ USE_SOURCE_PERMISSIONS)
endforeach(IMAGES ${AtlasImageFiles})
install(FILES ${AtlasImageFiles} DESTINATION bin/${ATLAS_NAME})

file(GLOB AtlasImageNrrdFiles "${ATLAS_SOURCE_DIR}/*.nrrd")
foreach(IMAGES ${AtlasImageNrrdFiles})
  get_filename_component(image ${IMAGES} NAME)
  file(COPY ${IMAGES} DESTINATION ${ATLAS_DIRECTORY}/ USE_SOURCE_PERMISSIONS)
endforeach(IMAGES ${AtlasImageNrrdFiles})
install(FILES ${AtlasImageNrrdFiles} DESTINATION bin/${ATLAS_NAME})

file(GLOB AtlasColorLookupTableFiles "${ATLAS_SOURCE_DIR}/*.txt")
foreach(LUTS ${AtlasColorLookupTableFiles})
  get_filename_component(lut ${LUTS} NAME)
  file(COPY ${LUTS} DESTINATION ${ATLAS_DIRECTORY}/ USE_SOURCE_PERMISSIONS)
endforeach(LUTS ${AtlasColorLookupTableFiles})
install( FILES ${AtlasColorLookupTableFiles} DESTINATION bin/${ATLAS_NAME} )

file(GLOB SlicerColorTableFiles "${ATLAS_SOURCE_DIR}/*.ctbl")
foreach(LUTS ${SlicerColorTableFiles})
  get_filename_component(lut ${LUTS} NAME)
  file(COPY ${LUTS} DESTINATION ${ATLAS_DIRECTORY}/ USE_SOURCE_PERMISSIONS)
endforeach(LUTS ${SlicerColorTableFiles})
install( FILES ${SlicerColorTableFiles} DESTINATION bin/${ATLAS_NAME} )

file(GLOB AtlasColorLookupTableFiles "${ATLAS_SOURCE_DIR}/*.fcsv")
foreach(LUTS ${AtlasColorLookupTableFiles})
  get_filename_component(lut ${LUTS} NAME)
  file(COPY ${LUTS} DESTINATION ${ATLAS_DIRECTORY}/ USE_SOURCE_PERMISSIONS)
endforeach(LUTS ${AtlasColorLookupTableFiles})
install( FILES ${AtlasColorLookupTableFiles} DESTINATION bin/${ATLAS_NAME} )

file(GLOB AtlasColorLookupTableFiles "${ATLAS_SOURCE_DIR}/*.csv")
foreach(LUTS ${AtlasColorLookupTableFiles})
  get_filename_component(lut ${LUTS} NAME)
  file(COPY ${LUTS} DESTINATION ${ATLAS_DIRECTORY}/ USE_SOURCE_PERMISSIONS)
endforeach(LUTS ${AtlasColorLookupTableFiles})
install( FILES ${AtlasColorLookupTableFiles} DESTINATION bin/${ATLAS_NAME} )

file(GLOB AtlasColorLookupTableFiles "${ATLAS_SOURCE_DIR}/modelFiles/*.gz")
foreach(LUTS ${AtlasColorLookupTableFiles})
  get_filename_component(lut ${LUTS} NAME)
  file(COPY ${LUTS} DESTINATION ${ATLAS_DIRECTORY}/modelFiles/ USE_SOURCE_PERMISSIONS)
endforeach(LUTS ${AtlasColorLookupTableFiles})
install( FILES ${AtlasColorLookupTableFiles} DESTINATION bin/${ATLAS_NAME}/modelFiles )

file(GLOB AtlasColorLookupTableFiles "${ATLAS_SOURCE_DIR}/T1OnlyModels/*.gz")
foreach(LUTS ${AtlasColorLookupTableFiles})
  get_filename_component(lut ${LUTS} NAME)
  file(COPY ${LUTS} DESTINATION ${ATLAS_DIRECTORY}/T1OnlyModels/ USE_SOURCE_PERMISSIONS)
endforeach(LUTS ${AtlasColorLookupTableFiles})
install( FILES ${AtlasColorLookupTableFiles} DESTINATION bin/${ATLAS_NAME}/T1OnlyModels )

file(GLOB AtlasColorLookupTableFiles "${ATLAS_SOURCE_DIR}/probabilityMaps/*.nii.gz")
foreach(LUTS ${AtlasColorLookupTableFiles})
  get_filename_component(lut ${LUTS} NAME)
  file(COPY ${LUTS} DESTINATION ${ATLAS_DIRECTORY}/probabilityMaps/ USE_SOURCE_PERMISSIONS)
endforeach(LUTS ${AtlasColorLookupTableFiles})
install( FILES ${AtlasColorLookupTableFiles} DESTINATION bin/${ATLAS_NAME}/probabilityMaps )

file(GLOB AtlasColorLookupTableFiles "${ATLAS_SOURCE_DIR}/spatialImages/*.nii.gz")
foreach(LUTS ${AtlasColorLookupTableFiles})
  get_filename_component(lut ${LUTS} NAME)
  file(COPY ${LUTS} DESTINATION ${ATLAS_DIRECTORY}/spatialImages/ USE_SOURCE_PERMISSIONS)
endforeach(LUTS ${AtlasColorLookupTableFiles})
install( FILES ${AtlasColorLookupTableFiles} DESTINATION bin/${ATLAS_NAME}/spatialImages )

file(GLOB AtlasSurfaceToolsFile "${ATLAS_SOURCE_DIR}/SurfaceAtlas/*.vtk")
foreach(SURF ${AtlasSurfaceToolsFile})
  get_filename_component(surf ${SURF} NAME)
  file(COPY ${SURF} DESTINATION ${ATLAS_DIRECTORY}/SurfaceAtlas/ USE_SOURCE_PERMISSIONS)
endforeach(SURF ${AtlasSurfaceToolsFile})
install( FILES ${AtlasSurfaceToolsFile} DESTINATION bin/${ATLAS_NAME}/SurfaceAtlas )

file(GLOB BCD20111119_BCD "${ATLAS_SOURCE_DIR}/20111119_BCD/*_50Lmks.*")
foreach(LMK_FILES ${BCD20111119_BCD})
  get_filename_component(surf ${LMK_FILES} NAME)
  file(COPY ${LMK_FILES} DESTINATION ${ATLAS_DIRECTORY}/20111119_BCD/ USE_SOURCE_PERMISSIONS)
endforeach()
install( FILES ${BCD20111119_BCD} DESTINATION bin/${ATLAS_NAME}/20111119_BCD )




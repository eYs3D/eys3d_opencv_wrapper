#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "opencv_world" for configuration "Release"
set_property(TARGET opencv_world APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(opencv_world PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/x64/vc15/lib/opencv_world342.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "vtkRenderingOpenGL2;vtkCommonCore;vtksys;vtkCommonDataModel;vtkCommonMath;vtkCommonMisc;vtkCommonSystem;vtkCommonTransforms;vtkCommonExecutionModel;vtkIOImage;vtkDICOMParser;vtkmetaio;vtkzlib;vtkjpeg;vtkpng;vtktiff;vtkImagingCore;vtkRenderingCore;vtkCommonColor;vtkCommonComputationalGeometry;vtkFiltersCore;vtkFiltersGeneral;vtkFiltersGeometry;vtkFiltersSources;vtkglew;vtkInteractionStyle;vtkFiltersExtraction;vtkFiltersStatistics;vtkImagingFourier;vtkalglib;vtkRenderingLOD;vtkFiltersModeling;vtkIOPLY;vtkIOCore;vtkFiltersTexture;vtkRenderingFreeType;vtkfreetype;vtkIOExport;vtkRenderingGL2PSOpenGL2;vtkgl2ps;vtkIOGeometry;vtkIOLegacy"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/x64/vc15/bin/opencv_world342.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS opencv_world )
list(APPEND _IMPORT_CHECK_FILES_FOR_opencv_world "${_IMPORT_PREFIX}/x64/vc15/lib/opencv_world342.lib" "${_IMPORT_PREFIX}/x64/vc15/bin/opencv_world342.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

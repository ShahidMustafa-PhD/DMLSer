cmake_minimum_required (VERSION 3.28)
project (DMLSer)

# only on newer GCCs: -ftemplate-backtrace-limit=0
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11  -Wall -DM_PI=3.14159265358979323846 -D_GLIBCXX_USE_C99 -DHAS_BOOL -DNOGDI -DBOOST_ASIO_DISABLE_KQUEUE")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -DSLIC3R_DEBUG")


if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.7.0)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7.3)
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-inline-small-functions")
    endif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7.3)
endif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.7.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

IF(CMAKE_HOST_APPLE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DBOOST_NO_CXX11_RVALUE_REFERENCES -DBOOST_THREAD_USES_MOVE")
 

set(CMAKE_EXE_LINKER_FLAGS "-framework IOKit -framework CoreFoundation -lc++")
ELSE(CMAKE_HOST_APPLE)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
ENDIF(CMAKE_HOST_APPLE)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
find_package(Boost REQUIRED system thread filesystem)

set(LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/src/)
set(UTILITIES ${CMAKE_CURRENT_SOURCE_DIR}/src/Utilities)
set(CORE ${CMAKE_CURRENT_SOURCE_DIR}/src/Core)
include_directories(${LIBDIR})
include_directories(${CORE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/standalone/)
include_directories(${UTILITIES})
include_directories(${UTILITIES}/admesh/)
include_directories(${UTILITIES}/BSpline/)
include_directories(${UTILITIES}/expat/)
include_directories(${UTILITIES}/poly2tri/)
include_directories(${UTILITIES}/poly2tri/sweep)
include_directories(${UTILITIES}/poly2tri/common)
include_directories(${UTILITIES}/clipper/)
#include_directories(${LIBDIR}/SLMPrint)

add_library(dmls STATIC
    ${CORE}/BoundingBox.cpp
    ${CORE}/BridgeDetector.cpp
    ${CORE}/ClipperUtils.cpp
    ${CORE}/Config.cpp
    ${CORE}/ExPolygon.cpp
    ${CORE}/ExPolygonCollection.cpp
    ${CORE}/Extruder.cpp
    ${CORE}/ExtrusionEntity.cpp
    ${CORE}/ExtrusionEntityCollection.cpp
    ${CORE}/Fill/Fill.cpp
    ${CORE}/Fill/Fill3DHoneycomb.cpp
    ${CORE}/Fill/FillConcentric.cpp
    ${CORE}/Fill/FillHoneycomb.cpp
    ${CORE}/Fill/FillPlanePath.cpp
    ${CORE}/Fill/FillRectilinear.cpp
    ${CORE}/Fill/FillSLMisland.cpp
    ${CORE}/Flow.cpp
    ${CORE}/Geometry.cpp
    ${CORE}/IO.cpp
    ${CORE}/IO/AMF.cpp
    ${CORE}/IO/TMF.cpp
    ${CORE}/Layer.cpp
    ${CORE}/LayerRegion.cpp
    ${CORE}/LayerRegionFill.cpp
    ${CORE}/LayerHeightSpline.cpp
    ${CORE}/Line.cpp
    ${CORE}/Model.cpp
    ${CORE}/MultiPoint.cpp
    ${CORE}/PerimeterGenerator.cpp
    ${CORE}/PlaceholderParser.cpp
    ${CORE}/Point.cpp
    ${CORE}/Polygon.cpp
    ${CORE}/Polyline.cpp
    ${CORE}/PolylineCollection.cpp
    ${CORE}/Print.cpp
    ${CORE}/PrintConfig.cpp
    ${CORE}/PrintObject.cpp
    ${CORE}/PrintRegion.cpp
    ${CORE}/SLMPrint.cpp
    ${CORE}/SlicingAdaptive.cpp
    ${CORE}/Surface.cpp
    ${CORE}/SurfaceCollection.cpp
    ${CORE}/SVG.cpp
    ${CORE}/TriangleMesh.cpp
    ${CORE}/utils.cpp
    ${CORE}/Zip/ZipArchive.cpp
    ${CORE}/SLMSlicer.cpp
)

add_library(BSpline STATIC
    ${UTILITIES}/BSpline/BSpline.cpp
)

add_library(admesh STATIC
    ${UTILITIES}/admesh/connect.c
    ${UTILITIES}/admesh/normals.c
    ${UTILITIES}/admesh/shared.c
    ${UTILITIES}/admesh/stl_io.c
    ${UTILITIES}/admesh/stlinit.c
    ${UTILITIES}/admesh/util.c
)
set_property(TARGET admesh PROPERTY C_STANDARD 99)

add_library(clipper STATIC ${UTILITIES}/clipper/clipper.cpp)
add_library(expat STATIC
    ${UTILITIES}/expat/xmlparse.c
    ${UTILITIES}/expat/xmlrole.c
    ${UTILITIES}/expat/xmltok.c
)
add_library(polypartition STATIC ${UTILITIES}/polypartition.cpp)
add_library(poly2tri STATIC
    ${UTILITIES}/poly2tri/common/shapes.cc
    ${UTILITIES}/poly2tri/sweep/advancing_front.cc
    ${UTILITIES}/poly2tri/sweep/cdt.cc
    ${UTILITIES}/poly2tri/sweep/sweep_context.cc
    ${UTILITIES}/poly2tri/sweep/sweep.cc
)




add_executable(DMLSer Main.cpp)
set_target_properties(DMLSer PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(DMLSer PROPERTIES LINK_SEARCH_END_STATIC 1)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
include_directories(${Boost_INCLUDE_DIRS})

#find_package(wxWidgets)
#disable wx for the time being - we're not building any of the gui yet
target_link_libraries (dmls admesh BSpline clipper expat polypartition poly2tri)

target_link_libraries (DMLSer PRIVATE dmls  Boost::thread  Boost::system  Boost::filesystem)

# Windows needs a compiled component for Boost.nowide
IF (WIN32)
    add_library(boost-nowide STATIC ${LIBDIR}/boost/nowide/iostream.cpp)
    target_link_libraries(DMLSer boost-nowide)

ENDIF(WIN32)


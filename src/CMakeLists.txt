#
# Copyright Ange Optimization 2014
#

add_library(qdatacube SHARED
    abstractaggregator.cpp
    abstractfilter.cpp
    abstractformatter.cpp
    andfilter.cpp
    cell.cpp
    columnaggregator.cpp
    columnsumformatter.cpp
    countformatter.cpp
    datacube.cpp
    datacubeselection.cpp
    datacubeview.cpp
    filterbyaggregate.cpp
    orfilter.cpp
)
target_link_libraries(qdatacube Qt5::Core Qt5::Widgets)
generate_export_header(qdatacube)
set_property(TARGET qdatacube PROPERTY VERSION "${QDATACUBE_SO_VERSION}.0.0")
set_property(TARGET qdatacube PROPERTY SOVERSION "${QDATACUBE_SO_VERSION}")

install(TARGETS qdatacube EXPORT QDatacubeTargets
    RUNTIME DESTINATION "bin"
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    INCLUDES DESTINATION "include"
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/qdatacube_export.h
    abstractaggregator.h
    abstractfilter.h
    abstractformatter.h
    andfilter.h
    columnaggregator.h
    columnsumformatter.h
    countformatter.h
    datacube.h
    datacubeselection.h
    datacubeview.h
    filterbyaggregate.h
    orfilter.h
    DESTINATION "include/qdatacube"
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/QDatacubeConfigVersion.cmake
    VERSION ${QDATACUBE_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(
    QDatacubeConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/QDatacubeConfig.cmake"
    @ONLY
)

set(ConfigPackageLocation lib/cmake/QDatacube)
install(EXPORT QDatacubeTargets
    FILE QDatacubeTargets.cmake
    DESTINATION ${ConfigPackageLocation})
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QDatacubeConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QDatacubeConfigVersion.cmake"
    DESTINATION  ${ConfigPackageLocation})

add_subdirectory(test)

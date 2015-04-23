if(ALBERTA_FOUND)
  set(ALBERTA_PROGRAMS
    test-alberta
    test-alberta-1-1
    test-alberta-1-2
    test-alberta-2-2
    test-alberta-1-3
    test-alberta-2-3
    test-alberta-3-3
    test-alberta-generic)
endif(ALBERTA_FOUND)

if(ALUGRID_FOUND)
  set(ALUGRID_PROGRAMS test-alugrid)
endif(ALUGRID_FOUND)

if(UG_FOUND)
  set(UG_PROGRAMS
    test-ug
    test-parallel-ug)
endif(UG_FOUND)

if(UG_FOUND AND ALUGRID_FOUND)
  set(DGFALUGRID_UG_PROGRAMS test-dgfalu-uggrid-combination)
endif(UG_FOUND AND ALUGRID_FOUND)
set(GRIDDIM 2)
set(WORLDDIM 2)

#
## Some parameters for the geogrid test
#
set(COORDFUNCTION "IdenticalCoordFunction<double,${GRIDDIM}>")
# set(COORDFUNCTION "Helix")
# set(COORDFUNCTION  "ThickHelix")
# set(COORDFUNCTION "DGFCoordFunction\\<${GRIDDIM},${GRIDDIM}\\>")
# set(COORDFUNCTION "DGFCoordFunction\\<2,3\\>")
set(CACHECOORDFUNCTION 0)

#
## define the lists of tests to build and run
#

# tests where program to build and program to run are equal
set(TESTS
  test-geogrid test-identitygrid test-oned test-sgrid test-yaspgrid
  ${ALBERTA_PROGRAMS} ${ALUGRID_PROGRAMS} ${UG_PROGRAMS}
  ${DGFALUGRID_UG_PROGRAMS} test-mcmg-geogrid
  testiteratorranges test-hierarchicsearch)

set_property(DIRECTORY APPEND PROPERTY
  COMPILE_DEFINITIONS "DUNE_GRID_EXAMPLE_GRIDS_PATH=\"${PROJECT_SOURCE_DIR}/doc/grids/\"")

add_executable(test-geogrid test-geogrid.cc)
add_executable(test-identitygrid test-identitygrid.cc)
add_executable(test-oned test-oned.cc)
add_executable(test-mcmg-geogrid test-mcmg-geogrid.cc)
add_executable(test-sgrid test-sgrid.cc)
add_executable(test-yaspgrid test-yaspgrid.cc)
add_dune_mpi_flags(test-yaspgrid)

set_property(TARGET test-sgrid test-oned test-yaspgrid test-geogrid APPEND
  PROPERTY COMPILE_DEFINITIONS "GRIDDIM=${GRIDDIM}" "WORLDDIM=${GRIDDIM}" "YASPGRID")

set_property(TARGET test-geogrid APPEND PROPERTY COMPILE_DEFINITIONS
  COORDFUNCTION=${COORDFUNCTION} CACHECOORDFUNCTION=${CACHECOORDFUNCTION})

if(ALBERTA_FOUND)
  add_executable(test-alberta test-alberta.cc)
  add_dune_alberta_flags(test-alberta WORLDDIM ${GRIDDIM})
  add_executable(test-alberta-1-1 test-alberta.cc)
  add_dune_alberta_flags(test-alberta-1-1 GRIDDIM 1 WORLDDIM 1)
  add_executable(test-alberta-1-1-no-deprecated test-alberta.cc)
  add_dune_alberta_flags(test-alberta-1-1-no-deprecated GRIDDIM 1 WORLDDIM 1)
  set_property(TARGET test-alberta-1-1-no-deprecated APPEND PROPERTY COMPILE_DEFINITIONS
    DISABLE_DEPRECATED_METHOD_CHECK=1)
  add_executable(test-alberta-1-2 test-alberta.cc)
  add_dune_alberta_flags(test-alberta-1-2 WORLDDIM 2)
  add_executable(test-alberta-2-2 test-alberta.cc)
  add_dune_alberta_flags(test-alberta-2-2 WORLDDIM 2)
  add_executable(test-alberta-1-3 test-alberta.cc)
  add_dune_alberta_flags(test-alberta-1-3 WORLDDIM 3)
  add_executable(test-alberta-2-3 test-alberta.cc)
  add_dune_alberta_flags(test-alberta-2-3 WORLDDIM 3)
  add_executable(test-alberta-3-3 test-alberta.cc)
  add_dune_alberta_flags(test-alberta-3-3 WORLDDIM 3)
  add_executable(test-alberta-generic test-alberta.cc)
  add_dune_alberta_flags(test-alberta-generic USE_GENERIC WORLDDIM 2)
endif(ALBERTA_FOUND)

if(ALUGRID_FOUND)
  add_executable(test-alugrid test-alugrid.cc)
  add_dune_alugrid_flags(test-alugrid)
endif(ALUGRID_FOUND)

if(UG_FOUND)
  add_executable(test-ug test-ug.cc)
  add_dune_ug_flags(test-ug)
  add_executable(test-parallel-ug test-parallel-ug.cc)
  add_dune_ug_flags(test-parallel-ug)
  # test-loadbalancing
  add_executable(test-loadbalancing test-loadbalancing.cc)
  add_dune_ug_flags(test-loadbalancing)
  add_dune_parmetis_flags(test-loadbalancing)
endif(UG_FOUND)

if(ALUGRID_FOUND AND UG_FOUND)
  add_executable(test-dgfalu-uggrid-combination test-dgfalu-uggrid-combination.cc)
  add_dune_ug_flags(test-dgfalu-uggrid-combination)
  add_dune_alugrid_flags(test-dgfalu-uggrid-combination)
endif(ALUGRID_FOUND AND UG_FOUND)

add_executable(testiteratorranges testiteratorranges.cc)
add_dune_mpi_flags(testiteratorranges)

add_executable(test-hierarchicsearch test-hierarchicsearch.cc)
add_dune_mpi_flags(test-hierarchicsearch)


# tell gridcheck to use old entity and intersections interface for unported grids
set_property(TARGET
  ${ALUGRID_PROGRAMS}
  APPEND PROPERTY COMPILE_DEFINITIONS "DUNE_GRID_CHECK_USE_DEPRECATED_ENTITY_AND_INTERSECTION_INTERFACE"
  )


foreach(_exe ${TESTS})
  target_link_libraries(${_exe} "dunegrid" ${DUNE_LIBS})
  add_test(${_exe} ${_exe})
endforeach(_exe ${TESTS})

# define HAVE_DUNE_GRID for the dgfparser
set_property(TARGET ${TESTS} APPEND PROPERTY
  COMPILE_DEFINITIONS HAVE_DUNE_GRID=1)

set(SOURCES
  basicunitcube.hh
  check-albertareader.cc
  check-albertareader.hh
  checkadaptation.cc
  checkadaptation.hh
  checkcommunicate.cc
  checkcommunicate.hh
  checkentitylifetime.hh
  checkentityseed.cc
  checkentityseed.hh
  checkgeometry.cc
  checkgeometry.hh
  checkgeometryinfather.cc
  checkgeometryinfather.hh
  checkindexset.cc
  checkindexset.hh
  checkintersectionit.cc
  checkintersectionit.hh
  checkintersectionlifetime.hh
  checkiterators.cc
  checkiterators.hh
  checkjacobians.cc
  checkjacobians.hh
  checkpartition.cc
  checkpartition.hh
  checktwists.cc
  checktwists.hh
  functions.hh
  gridcheck.cc
  gridcheck.hh
  staticcheck.hh)

# install the tests as we want to support testing 3rdparty grids with installed dune-grid
install(FILES ${SOURCES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/grid/test)

# clean up additional files during make clean. Unfortunately it is not possible with
# cmake to use search patterns here. Automake wanted to clean the following files:
# *.gcda *.gcno semantic.cache simplex-testgrid*.dgf.* cube-testgrid*.dgf.* dgfparser.log
# add files to the list as they appear.
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "semantic.cache;dgfparser.log")

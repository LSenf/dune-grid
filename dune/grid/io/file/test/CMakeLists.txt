set(GRIDDIM 2)
set(GRIDTYPE YASPGRID)
add_definitions("-DDUNE_GRID_EXAMPLE_GRIDS_PATH=\"${PROJECT_SOURCE_DIR}/doc/grids/\"")

set(TESTS
  gmshtest
  gnuplottest
  printgridtest)

set(CONSISTENT_VTK_TESTS
  conformvolumevtktest
  nonconformboundaryvtktest
  vtktest
  vtksequencetest)

set(VTK_TESTS ${CONSISTENT_VTK_TESTS} subsamplingvtktest)

if(AMIRAMESH_FOUND AND PSURFACE_FOUND)
  set(AMIRAMESH_TESTS amirameshtest)
endif(AMIRAMESH_FOUND AND PSURFACE_FOUND)

if(ALBERTA_FOUND)
  set(ALBERTA_TESTS gmshtest-alberta2d gmshtest-alberta3d)
endif(ALBERTA_FOUND)

if(ALUGRID_FOUND)
  set(ALUGRID_TESTS gmshtest-alugrid)
endif(ALUGRID_FOUND)

if(UG_FOUND)
  set(UG_TESTS starcdreadertest)
endif(UG_FOUND)

set(BUILD_TESTS ${TESTS} ${UG_TESTS} ${CONSISTENT_VTK_TESTS} ${AMIRAMESH_TESTS})
set(ALLTESTS ${BUILD_TESTS}  ${ALUGRID_TESTS} ${ALBERTA_TESTS})

foreach(_test ${BUILD_TESTS})
  add_executable(${_test} ${_test}.cc)
  target_link_libraries(${_test} dunegrid ${DUNE_LIBS})
endforeach(_test ${BUILD_TESTS})

add_executable(subsamplingvtktest subsamplingvtktest.cc test-linking.cc)
target_link_libraries(subsamplingvtktest dunegrid ${DUNE_LIBS})

foreach(_test ${ALLTESTS})
  add_test(${_test} ${_test})
endforeach(_test ${ALLTESTS})

if(MPI_FOUND)
  add_test(NAME mpivtktest WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND mpirun -np 2 ./vtktest)
  set_tests_properties(mpivtktest PROPERTIES SKIP_RETURN_CODE 77)
endif(MPI_FOUND)

foreach(_test ${AMIRAMESH_TESTS})
  add_dune_amiramesh_flags(${_test})
  add_dune_ug_flags(${_test})
  add_dune_alugrid_flags(${_test})
  add_dune_alberta_flags(${_test} GRIDDIM 2)
  add_dune_psurface_flags(${_test})
endforeach(_test ${AMIRAMESH_TESTS})

add_dune_mpi_flags(${VTK_TESTS})

add_executable(gmshtest-alugrid gmshtest.cc)
set_property(
  TARGET gmshtest-alugrid
  APPEND PROPERTY COMPILE_DEFINITIONS "DUNE_GRID_CHECK_USE_DEPRECATED_ENTITY_AND_INTERSECTION_INTERFACE"
  )
add_dune_alugrid_flags(gmshtest-alugrid)

add_executable(gmshtest-alberta2d gmshtest.cc)
add_dune_alberta_flags(gmshtest-alberta2d WORLDDIM 2)
add_dune_grape_flags(gmshtest-alberta2d)

add_executable(gmshtest-alberta3d gmshtest.cc)
add_dune_alberta_flags(gmshtest-alberta3d WORLDDIM 3)
add_dune_grape_flags(gmshtest-alberta3d)

set_tests_properties(vtktest PROPERTIES SKIP_RETURN_CODE 77)

foreach(_test ${UG_TESTS})
  add_dune_ug_flags(${_test})
endforeach(_test ${UG_TESTS})

# We do not want want to build the tests during make all,
# but just build them on demand
add_directory_test_target(_test_target)
add_dependencies(${_test_target} ${ALLTESTS})

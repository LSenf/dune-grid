set(TESTS
  globalindexsettest
  tensorgridfactorytest
  structuredgridfactorytest
  vertexordertest
  persistentcontainertest)

foreach(_T ${TESTS})
  add_executable(${_T} ${_T}.cc)
  target_link_libraries(${_T} dunegrid ${DUNE_LIBS})
  add_test(${_T} ${_T})
endforeach(_T ${TESTS})

add_dune_ug_flags(${TESTS})
add_dune_mpi_flags(structuredgridfactorytest tensorgridfactorytest)
add_dune_alugrid_flags(vertexordertest persistentcontainertest)

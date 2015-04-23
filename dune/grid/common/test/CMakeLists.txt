set(TESTS scsgmappertest)

if(UG_FOUND)
  set(TESTS
     ${TESTS} mcmgmappertest)
endif(UG_FOUND)

foreach(_t ${TESTS})
  add_executable(${_t} ${_t}.cc)
  target_link_libraries(${_t} dunegrid ${DUNE_LIBS})
  add_test(${_t} ${_t})
endforeach(_t ${TESTS})

if(UG_FOUND)
    add_dune_ug_flags(mcmgmappertest)
endif(UG_FOUND)

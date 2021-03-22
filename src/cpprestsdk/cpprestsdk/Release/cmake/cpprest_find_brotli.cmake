function(cpprest_find_brotli)
  if(TARGET cpprestsdk_brotli_internal)
    return()
  endif()


  find_package(PkgConfig)
  pkg_check_modules(BROTLIENC libbrotlienc)
  pkg_check_modules(BROTLIDEC libbrotlidec)
  if(BROTLIDEC_FOUND AND BROTLIENC_FOUND)
	  target_link_libraries(cpprest PRIVATE ${BROTLIDEC_LDFLAGS} ${BROTLIENC_LDFLAGS})
  else(BROTLIDEC_FOUND AND BROTLIENC_FOUND)
    find_package(unofficial-brotli REQUIRED)
    add_library(cpprestsdk_brotli_internal INTERFACE)
    target_link_libraries(cpprestsdk_brotli_internal INTERFACE unofficial::brotli::brotlienc unofficial::brotli::brotlidec unofficial::brotli::brotlicommon)
    target_link_libraries(cpprest PRIVATE cpprestsdk_brotli_internal)
  endif(BROTLIDEC_FOUND AND BROTLIENC_FOUND)

endfunction()

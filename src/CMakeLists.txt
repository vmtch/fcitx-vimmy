add_library(vimmy SHARED vimmy.cpp)
add_library(normal STATIC normal.cpp)

find_package(Fcitx5Core REQUIRED)
target_link_libraries(vimmy PRIVATE Fcitx5::Core)

target_link_libraries(vimmy PRIVATE normal)

set_target_properties(vimmy PROPERTIES PREFIX "")

install(TARGETS vimmy DESTINATION "/usr/lib64/fcitx5")

configure_file(vimmy-addon.conf.in ${CMAKE_BINARY_DIR}/share/fcitx5/addon/vimmy.conf @ONLY)
configure_file(vimmy.conf.in ${CMAKE_BINARY_DIR}/share/fcitx5/inputmethod/vimmy.conf @ONLY)
install(FILES "${CMAKE_BINARY_DIR}/share/fcitx5/addon/vimmy.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
install(FILES "${CMAKE_BINARY_DIR}/share/fcitx5/inputmethod/vimmy.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod")

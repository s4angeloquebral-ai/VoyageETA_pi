set(SRC
  voyage_eta_pi.cpp
  voyage_eta_pi.h
  voyage_eta_dialog.cpp
  voyage_eta_dialog.h
)

add_library(${PACKAGE_NAME}_pi SHARED ${SRC})

target_include_directories(${PACKAGE_NAME}_pi PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/ocpninclude
  ${CMAKE_SOURCE_DIR}/opencpn-libs/api-18
)

target_link_libraries(${PACKAGE_NAME}_pi PRIVATE ${wxWidgets_LIBRARIES})

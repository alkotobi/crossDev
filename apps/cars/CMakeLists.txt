# Cars app module — extends CrossDev with invoice, etc.
# Linked when BUILD_CARS_APP is ON. Provides strong registerAppHandlers.
add_library(app_cars STATIC
    app_handlers.cpp
    handlers/create_invoice_handler.cpp
)
target_include_directories(app_cars PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(app_cars PUBLIC excel_excel)
target_compile_options(app_cars PRIVATE -Wno-defaulted-function-deleted)
if(PLATFORM STREQUAL "windows")
    target_link_libraries(app_cars PRIVATE shell32)
endif()

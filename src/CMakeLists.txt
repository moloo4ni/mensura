add_library(mensura_logic STATIC)
target_sources(mensura_logic PRIVATE mensuradsp.cpp mensuradsp.h)
target_sources(mensura_logic PRIVATE mensuraparams.h beatgrid.cpp beatgrid.h)
set_target_properties(
    mensura_logic
    PROPERTIES POSITION_INDEPENDENT_CODE ON
               AUTOMOC ON
               CXX_VISIBILITY_PRESET hidden
               VISIBILITY_INLINES_HIDDEN ON
)
target_include_directories(mensura_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mensura_logic PUBLIC Fooyin::Core Qt6::Core)
target_compile_definitions(mensura_logic PUBLIC QT_USE_QSTRINGBUILDER)
target_compile_options(mensura_logic PRIVATE -Wall -Wextra)

create_fooyin_plugin(
    mensura
    NO_INSTALL
    DEPENDS mensura_logic Fooyin::Core
    SOURCES mensuraplugin.cpp mensuraplugin.h
)
install(TARGETS mensura LIBRARY DESTINATION ${MENSURA_PLUGIN_DIR})

# mensura_add_test(<name> [extra link libraries...]) builds tests/<name>.cpp
function(mensura_add_test name)
    add_executable(${name} ${name}.cpp)
    set_target_properties(${name} PROPERTIES AUTOMOC ON)
    target_link_libraries(${name} PRIVATE mensura_logic Qt6::Test ${ARGN})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

mensura_add_test(tst_mensuradsp)

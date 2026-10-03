# Compiler settings shared by all of our own targets (E-08, E-09).

if(MSVC)
    set(DRUMPROG_WARNING_FLAGS /W4 /WX /permissive-)
else()
    # Designated initializers deliberately leave members at their defaults; GCC would flag that.
    set(DRUMPROG_WARNING_FLAGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-missing-field-initializers -Werror)
endif()

# Applies warnings-as-errors to a target made only of our own sources.
function(drumprog_enable_warnings target)
    target_compile_options(${target} PRIVATE ${DRUMPROG_WARNING_FLAGS})
endfunction()

# Applies warnings-as-errors to single source files. Used for the JUCE app target,
# whose JUCE module sources are compiled into the same target and must keep
# JUCE's own warning level.
function(drumprog_enable_warnings_for_sources)
    set_source_files_properties(${ARGN} PROPERTIES COMPILE_OPTIONS "${DRUMPROG_WARNING_FLAGS}")
endfunction()

if(DRUMPROG_SANITIZERS)
    if(MSVC)
        message(FATAL_ERROR "DRUMPROG_SANITIZERS is only supported with GCC/Clang")
    endif()
    add_compile_options(-fsanitize=${DRUMPROG_SANITIZERS} -fno-omit-frame-pointer -fno-sanitize-recover=all)
    add_link_options(-fsanitize=${DRUMPROG_SANITIZERS})
endif()

if(DRUMPROG_COVERAGE)
    if(MSVC)
        message(FATAL_ERROR "DRUMPROG_COVERAGE is only supported with GCC/Clang")
    endif()
    add_compile_options(--coverage -O0 -fno-inline)
    add_link_options(--coverage)
endif()

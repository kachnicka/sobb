add_subdirectory(sobb)

if (WIN32)
    set_directory_properties(
        PROPERTIES
            VS_STARTUP_PROJECT sobb
    )
endif()

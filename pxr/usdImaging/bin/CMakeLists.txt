set(DIRS
    usdrender
    usdview
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

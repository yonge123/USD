set(DIRS
    usdview
    usdrender
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

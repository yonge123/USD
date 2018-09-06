set(DIRS
    usdview
    usdrender
    testusdview
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

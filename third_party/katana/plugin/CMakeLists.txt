if (KATANA_API_VIEWER_API_VERSION GREATER_EQUAL 3)
    add_subdirectory(hydraKatana)
endif()
add_subdirectory(pxrUsdIn)
add_subdirectory(pxrUsdInPrman)
add_subdirectory(pxrUsdInShipped)
add_subdirectory(vmp_usd)


### DO NOT DELETE THIS COMMENT: INSERT_ARCHETYPES_HERE ###

USE_CYCLUS("mbmore" "mytest")
USE_CYCLUS("mbmore" "behavior_functions")
USE_CYCLUS("mbmore" "RandomEnrich")
USE_CYCLUS("mbmore" "RandomSink")

INSTALL_CYCLUS_MODULE("mbmore" "./")

# install header files
FILE(GLOB h_files "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
INSTALL(FILES ${h_files} DESTINATION include/mbmore COMPONENT mbmore)

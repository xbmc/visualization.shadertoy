project(glew)

cmake_minimum_required(VERSION 2.6)
enable_language(CXX)

include(ExternalProject)

externalproject_add(glew
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    UPDATE_COMMAND ""
                    CONFIGURE_COMMAND ""
                    BUILD_COMMAND make
                                  LOCALCC=${CMAKE_C_COMPILER}
                                  SYSTEM=darwin
                                  GLEW_DEST=${CMAKE_INSTALL_PREFIX}
                                  "LOCALCFLAGS=${CMAKE_C_FLAGS}"
                                  "LOCALLDFLAGS=${CMAKE_LD_FLAGS}"
                    INSTALL_COMMAND ""
                    BUILD_IN_SOURCE 1)

install(CODE "execute_process(COMMAND make -C ${PROJECT_SOURCE_DIR} install
                                      LOCALCC=${CMAKE_C_COMPILER}
                                      SYSTEM=darwin
                                      GLEW_DEST=${CMAKE_INSTALL_PREFIX}
                                      \"LOCALCFLAGS=${CMAKE_C_FLAGS}\"
                                      \"LOCALLDFLAGS=${CMAKE_LD_FLAGS}\")")

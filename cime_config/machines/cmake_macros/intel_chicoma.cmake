set(PIO_FILESYSTEM_HINTS "lustre")
if (MPILIB STREQUAL impi)
  set(MPICC "mpiicc")
  set(MPICXX "mpiicpc")
  set(MPIFC "mpiifort")
endif()
if (NOT MPILIB STREQUAL impi)
  set(MPICXX "mpic++")
endif()
string(APPEND SLIBS " -lpthread")
string(APPEND CXX_LIBS " -lstdc++")

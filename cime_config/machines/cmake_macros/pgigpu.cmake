string(APPEND CFLAGS " -time")
string(APPEND FFLAGS " -i4 -time -Mstack_arrays -Mextend -byteswapio -Mflushz -Kieee -Mallocatable=03")
if (compile_threaded)
  string(APPEND CFLAGS " -mp")
  string(APPEND CXXFLAGS " -mp")
  string(APPEND FFLAGS " -mp")
  string(APPEND LDFLAGS " -mp")
endif()
string(APPEND CPPDEFS " -DFORTRANUNDERSCORE -DNO_SHR_VMATH -DNO_R16 -DCPRPGI")
string(APPEND FC_AUTO_R8 " -r8")
if (DEBUG)
  string(APPEND FFLAGS " -O0 -g -Mbounds")
endif()
if (NOT DEBUG)
  string(APPEND CFLAGS " -O2 -Mvect=nosimd")
  string(APPEND FFLAGS " -O2 -Mvect=nosimd -DSUMMITDEV_PGI")
endif()
if (COMP_NAME STREQUAL datm)
  string(APPEND FFLAGS " -Mnovect")
endif()
if (COMP_NAME STREQUAL dlnd)
  string(APPEND FFLAGS " -Mnovect")
endif()
if (COMP_NAME STREQUAL drof)
  string(APPEND FFLAGS " -Mnovect")
endif()
if (COMP_NAME STREQUAL dwav)
  string(APPEND FFLAGS " -Mnovect")
endif()
if (COMP_NAME STREQUAL dice)
  string(APPEND FFLAGS " -Mnovect")
endif()
if (COMP_NAME STREQUAL docn)
  string(APPEND FFLAGS " -Mnovect")
endif()
if (COMP_NAME STREQUAL gptl)
  string(APPEND CPPDEFS " -DHAVE_SLASHPROC")
endif()
string(APPEND FFLAGS_NOOPT " -O0")
string(APPEND FIXEDFLAGS " -Mfixed")
string(APPEND FREEFLAGS " -Mfree")
set(HAS_F2008_CONTIGUOUS "FALSE")
set(SUPPORTS_CXX "TRUE")
set(CXX_LINKER "FORTRAN")
string(APPEND CXX_LIBS " -lstdc++")
string(APPEND LDFLAGS " -time -Wl,--allow-multiple-definition -acc")
set(MPICC "mpicc")
set(MPICXX "mpiCC")
set(MPIFC "mpif90")
set(SCC "pgcc")
set(SCXX "pgc++")
set(SFC "pgfortran")
set(PIO_FILESYSTEM_HINTS "gpfs")

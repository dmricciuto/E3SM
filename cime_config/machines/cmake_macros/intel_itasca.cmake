string(APPEND CMAKE_C_FLAGS " -O2 -I/soft/intel/x86_64/2013/composer_xe_2013/composer_xe_2013_sp1.3.174/mkl/include")
if (compile_threaded)
  string(APPEND CMAKE_C_FLAGS " -openmp")
endif()
string(APPEND CMAKE_Fortran_FLAGS " -I/soft/intel/x86_64/2013/composer_xe_2013/composer_xe_2013_sp1.3.174/mkl/include")
if (compile_threaded)
  string(APPEND CMAKE_Fortran_FLAGS " -openmp")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -openmp")
endif()
set(MPICC "mpiicc")
set(MPICXX "mpiicpc")
set(MPIFC "mpiifort")
string(APPEND CMAKE_EXE_LINKER_FLAGS " -openmp -fPIC -lpthread -lm")

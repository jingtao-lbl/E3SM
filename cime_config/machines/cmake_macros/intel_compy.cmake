if (COMP_NAME STREQUAL gptl)
  string(APPEND CMAKE_C_FLAGS " -DHAVE_SLASHPROC")
endif()
string(APPEND CPPDEFS " -DLINUX")
string(APPEND CMAKE_Fortran_FLAGS_DEBUG " -check all -ftrapuv -init=snan")
set(PIO_FILESYSTEM_HINTS "lustre")
string(APPEND CMAKE_EXE_LINKER_FLAGS " -lpmi ")
if (MPILIB STREQUAL impi)
  set(MPICC "mpiicc")
  set(MPICXX "mpiicpc")
  set(MPIFC "mpiifort")
endif()

if (NOT DEBUG)
  string(APPEND CFLAGS " -O2")
endif()
string(APPEND CXX_LIBS " -lstdc++")
if (NOT DEBUG)
  string(APPEND FFLAGS " -O2")
endif()
string(APPEND FFLAGS " -fallow-argument-mismatch -fallow-invalid-boz")
execute_process(COMMAND $ENV{NETCDF_PATH}/bin/nf-config --flibs OUTPUT_VARIABLE SHELL_CMD_OUTPUT_BUILD_INTERNAL_IGNORE0 OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND $ENV{NETCDF_PATH}/bin/nc-config --libs OUTPUT_VARIABLE SHELL_CMD_OUTPUT_BUILD_INTERNAL_IGNORE1 OUTPUT_STRIP_TRAILING_WHITESPACE)
string(APPEND SLIBS " ${SHELL_CMD_OUTPUT_BUILD_INTERNAL_IGNORE0} ${SHELL_CMD_OUTPUT_BUILD_INTERNAL_IGNORE1} -lblas -llapack")
if (NOT MPILIB STREQUAL mpi-serial)
  string(APPEND SLIBS " -L/nfs/gce/projects/climate/scratch -ladios2pio-nm-lib -L$ENV{ADIOS2_DIR}/lib -ladios2_c_mpi -ladios2_c -ladios2_cxx11_mpi -ladios2_cxx11 -ladios2_core_mpi -ladios2_core")
endif()
set(NETCDF_PATH "$ENV{NETCDF_PATH}")
set(PNETCDF_PATH "$ENV{PNETCDF_PATH}")
set(HDF5_PATH "$ENV{HDF5_PATH}")
set(ZLIB_PATH "$ENV{ZLIB_PATH}")

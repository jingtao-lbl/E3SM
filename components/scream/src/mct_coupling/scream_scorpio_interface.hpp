#ifndef SCREAM_SCORPIO_INTERFACE_HPP
#define SCREAM_SCORPIO_INTERFACE_HPP

#include "ekat/util/scream_utils.hpp"
#include "ekat/scream_types.hpp"

// TODO, figure out a better way to define netCDF output type for fields
#ifdef SCREAM_CONFIG_IS_CMAKE
#  ifdef SCREAM_DOUBLE_PRECISION
  const int PIO_REAL = 6;
#  else
  const int PIO_REAL = 5;
#  endif // SCREAM_DOUBLE_PRECISION
#else // SCREAM_CONFIG_IS_CMAKE
  const int PIO_REAL = 6;
#endif // SCREAM_CONFIG_IS_CMAKE
const int PIO_INT = 4;

using scream::Real;
using scream::Int;
namespace scream {
namespace scorpio {

  void eam_init_pio_subsystem(const int mpicom, const int compid, const bool local);
  void eam_init_finalize();
  void register_outfile(const std::string& filename);
  void sync_outfile(const std::string& filename);
  void register_dimension(const std::string& filename,const std::string& shortname, const std::string& longname, const int length);
  void register_variable(const std::string& filename,const std::string& shortname, const std::string& longname, const int numdims, const std::string (&var_dimensions)[numdims], const int dtype, const std::string& pio_decomp_tag);
  void eam_pio_enddef();
  void pio_update_time(const std::string &filename, const Real time);
  void grid_write_data_array(const std::string &filename, const std::string &varname, const Int dim1_length, const Real *hbuf);
  void grid_write_data_array(const std::string &filename, const std::string &varname, const Int dim_length[2], const Real hbuf[dim_length[0]][dim_length[1]]);
  void grid_write_data_array(const std::string &filename, const std::string &varname, const Int dim_length[3], const Real hbuf[dim_length[0]][dim_length[1]][dim_length[2]]);
  void grid_write_data_array(const std::string &filename, const std::string &varname, const Int dim_length[4], const Real hbuf[dim_length[0]][dim_length[1]][dim_length[2]][dim_length[3]]);
  void grid_write_data_array(const std::string &filename, const std::string &varname, const Int dim1_length, const Int *hbuf);
  void grid_write_data_array(const std::string &filename, const std::string &varname, const Int dim_length[2], const Int hbuf[dim_length[0]][dim_length[1]]);
  void grid_write_data_array(const std::string &filename, const std::string &varname, const Int dim_length[3], const Int hbuf[dim_length[0]][dim_length[1]][dim_length[2]]);
  void grid_write_data_array(const std::string &filename, const std::string &varname, const Int dim_length[4], const Int hbuf[dim_length[0]][dim_length[1]][dim_length[2]][dim_length[3]]);
  //void grid_write_data_array(const std::string &filename, const std::string &varname, const Int buf_length, const Int  &hbuf);

} // namespace scorpio
} // namespace scream

#endif // define SCREAM_SCORPIO_INTERFACE_HPP 

#include "diagnostics/relative_humidity.hpp"
#include "physics/share/physics_functions.hpp" // also for ETI not on GPUs
#include "physics/share/physics_saturation_impl.hpp"

namespace scream
{

//using physics = scream::physics::Functions<Real, DefaultDevice>;

// =========================================================================================
RelativeHumidityDiagnostic::RelativeHumidityDiagnostic (const ekat::Comm& comm, const ekat::ParameterList& params)
  : AtmosphereDiagnostic(comm,params)
{
  // Nothing to do here
}

// =========================================================================================
void RelativeHumidityDiagnostic::set_grids(const std::shared_ptr<const GridsManager> grids_manager)
{
  using namespace ekat::units;
  using namespace ShortFieldTagsNames;

  //using physics = scream::physics::Functions<Real, DefaultDevice>;  

  auto Q = kg/kg;
  Q.set_string("kg/kg");

  const auto& grid_name = m_params.get<std::string>("Grid");
  auto grid  = grids_manager->get_grid(grid_name);
  m_num_cols = grid->get_num_local_dofs(); // Number of columns on this rank
  m_num_levs = grid->get_num_vertical_levels();  // Number of levels per column

  FieldLayout scalar3d_layout_mid { {COL,LEV}, {m_num_cols,m_num_levs} };
  constexpr int ps = Pack::n;

  // The fields required for this diagnostic to be computed
  add_field<Required>("T_mid",          scalar3d_layout_mid, K,  grid_name, ps);
  add_field<Required>("p_mid",          scalar3d_layout_mid, Pa, grid_name, ps);
  add_field<Required>("qv",          scalar3d_layout_mid, Q,  grid_name, "tracers", ps);


  // Construct and allocate the diagnostic field
  FieldIdentifier fid (name(), scalar3d_layout_mid, K, grid_name);
  m_diagnostic_output = Field(fid);
  auto& C_ap = m_diagnostic_output.get_header().get_alloc_properties();
  C_ap.request_allocation(ps);
  m_diagnostic_output.allocate_view();
}
// =========================================================================================
void RelativeHumidityDiagnostic::initialize_impl(const RunType /* run_type */)
{
  auto ts = timestamp(); 
  m_diagnostic_output.get_header().get_tracking().update_time_stamp(ts);
}
// =========================================================================================
void RelativeHumidityDiagnostic::compute_diagnostic_impl()
{

  const auto npacks  = ekat::npack<Pack>(m_num_levs);
  auto theta = m_diagnostic_output.get_view<Pack**>();
  auto T_mid = get_field_in("T_mid").get_view<const Pack**>();
  auto p_mid = get_field_in("p_mid").get_view<const Pack**>();
  auto qv_mid   = get_field_in("qv").get_view<const Pack**>();
  const auto& RH         = m_diagnostic_output.get_view<Pack**>();
//  auto qv_sat_l;

  using physics = scream::physics::Functions<Real, DefaultDevice>;

  Kokkos::parallel_for("RelativeHumidityDiagnostic",
                       Kokkos::RangePolicy<>(0,m_num_cols*npacks),
                       KOKKOS_LAMBDA (const int& idx) {
      const int icol  = idx / npacks;
      const int jpack = idx % npacks;
//      int nk;
//      const auto range_pack = ekat::range<IntPack>(k*Pack::n);
//      const auto range_mask = range_pack < nk;
      using Smask = ekat::Mask<Pack::n>;
      Smask range_mask(true);
      //const bool range_mask = true;
      //theta(icol,jpack) = PF::calculate_theta_from_T(T_mid(icol,jpack),p_mid(icol,jpack));
      auto qv_sat_l = physics::qv_sat(T_mid(icol,jpack), p_mid(icol,jpack), false, range_mask);
      RH(icol,jpack) = qv_mid(icol,jpack)/qv_sat_l;

  });
  Kokkos::fence();
}
// =========================================================================================

} //namespace scream

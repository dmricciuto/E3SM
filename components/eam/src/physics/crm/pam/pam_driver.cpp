#include "pam_coupler.h"
#include "Dycore.h"
#include "Microphysics.h"
#include "SGS.h"
#include "radiation.h"
#include "pam_interface.h"
#include "perturb_temperature.h"
#include "gcm_forcing.h"
#include "pam_feedback.h"
#include "pam_state.h"
#include "pam_radiation.h"
#include "pam_statistics.h"
#include "sponge_layer.h"
#include "saturation_adjustment.h"
#include "broadcast_initial_gcm_column.h"
#include "surface_friction.h"

extern "C" void pam_driver() {
  //------------------------------------------------------------------------------------------------
  using yakl::intrinsics::abs;
  using yakl::intrinsics::maxval;
  auto &coupler = pam_interface::get_coupler();
  //------------------------------------------------------------------------------------------------
  // retreive coupler options
  auto nens          = coupler.get_option<int>("ncrms");
  auto gcm_nlev      = coupler.get_option<int>("gcm_nlev");
  auto crm_nz        = coupler.get_option<int>("crm_nz");
  auto crm_nx        = coupler.get_option<int>("crm_nx");
  auto crm_ny        = coupler.get_option<int>("crm_ny");
  auto gcm_dt        = coupler.get_option<double>("gcm_dt");
  auto crm_dt        = coupler.get_option<double>("crm_dt");
  auto is_first_step = coupler.get_option<bool>("is_first_step");
  coupler.set_option<real>("gcm_physics_dt",gcm_dt);
  coupler.set_option<std::string>("p3_lookup_data_path","./p3_data");
  //------------------------------------------------------------------------------------------------
  // Allocate the coupler state and retrieve host/device data managers
  coupler.allocate_coupler_state( crm_nz , crm_ny , crm_nx , nens );
  auto &dm_device = coupler.get_data_manager_device_readwrite();
  auto &dm_host   = coupler.get_data_manager_host_readwrite();
  //------------------------------------------------------------------------------------------------
  // Create objects for dycor, microphysics, and turbulence and initialize them
  Microphysics micro;
  SGS          sgs;
  Dycore       dycore;
  Radiation    rad;
  micro .init(coupler);
  sgs   .init(coupler);
  dycore.init(coupler);
  rad   .init(coupler);
  //------------------------------------------------------------------------------------------------

  // update coupler GCM state with input GCM state
  pam_state_update_gcm_state(coupler);

  // set CRM dry density using gcm_density_dry (set in pam_state_update_gcm_state)
  modules::broadcast_initial_gcm_column_dry_density(coupler); 

  // Copy input CRM state (saved by the GCM) to coupler
  pam_state_copy_input_to_coupler(coupler);

  // Copy input rad tendencies to coupler (also sets up vertical grid)
  pam_radiation_copy_input_to_coupler(coupler);

  // initialize rad output variables
  pam_radiation_init(coupler);

  // initialize stat variables
  pam_statistics_init(coupler);

  // Define hydrostasis (only for PAM-A/AWFL)
  coupler.update_hydrostasis();

  // Compute CRM forcing tendencies
  modules::compute_gcm_forcing_tendencies(coupler);

  // initilize quantities for surface "psuedo-friction"
  auto input_tau  = dm_host.get<real const,1>("input_tau00").createDeviceCopy();
  auto input_bflx = dm_host.get<real const,1>("input_bflxls").createDeviceCopy();
  modules::surface_friction_init(coupler, input_tau, input_bflx);

  // Perturb the CRM at the beginning of the run
  if (is_first_step) {
    auto gcolp = dm_host.get<int const,1>("gcolp").createDeviceCopy();
    modules::perturb_temperature( coupler , gcolp );
  }
  //------------------------------------------------------------------------------------------------
  auto crm_zint          = dm_device.get<real,2>("vertical_interface_height" );
  auto crm_zmid          = dm_device.get<real,2>("vertical_midpoint_height" );
  auto crm_rho_d         = dm_device.get<real,4>("density_dry");
  auto crm_temp          = dm_device.get<real,4>("temp");
  auto crm_qv            = dm_device.get<real,4>("water_vapor");
  auto crm_qc            = dm_device.get<real,4>("cloud_water");
  auto crm_qi            = dm_device.get<real,4>("ice");
  auto crm_pmid          = coupler.compute_pressure_array();
  auto input_tl   = dm_host.get<real const,2>("input_tl").createDeviceCopy();
  auto input_ql   = dm_host.get<real const,2>("input_ql").createDeviceCopy();
  // auto input_qccl = dm_host.get<real const,2>("input_qccl").createDeviceCopy();
  // auto input_qiil = dm_host.get<real const,2>("input_qiil").createDeviceCopy();
  auto input_pmid = dm_host.get<real const,2>("input_pmid").createDeviceCopy();
  auto state_temperature   = dm_host.get<real const,4>("state_temperature").createDeviceCopy();
  auto state_qv            = dm_host.get<real const,4>("state_qv").createDeviceCopy();
  auto state_qc            = dm_host.get<real const,4>("state_qc").createDeviceCopy();
  auto state_qi            = dm_host.get<real const,4>("state_qi").createDeviceCopy();
  for (int k=0; k<crm_nz; k++) {
    int k_gcm = crm_nz-1-k;
    int i = 0;
    int j = 0;
    int iens = 0;
    // for (int i=0; i<crm_nx; i++) {
      std::cout <<"WHDEBUG0 "
                <<"  i:"<<i 
                // <<"  j:"<<j 
                <<"  k:"<<k 
                // <<"  icrm:"<<iens
                <<"  crm_zint:"  <<crm_zint  (k,iens)
                <<"  crm_zmid:"  <<crm_zmid  (k,iens)              
                <<"  crm_temp:"  <<crm_temp  (k,j,i,iens)
                // <<"  state_temp:"<<state_temperature  (k,j,i,iens)
                // <<"  input_tl:"  <<input_tl  (k_gcm,iens)                
                // <<"  crm_qv:"    <<crm_qv    (k,j,i,iens)
                // <<"  state_qv:"  <<state_qv  (k,j,i,iens)
                // <<"  input_ql:"  <<input_ql  (k_gcm,iens)                
                // <<"  crm_qc:"    <<crm_qc    (k,j,i,iens)
                // // <<"  input_qccl:"<<input_qccl(k_gcm,iens)
                // <<"  state_qc:"<<state_qc(k,j,i,iens)
                // <<"  crm_qi:"    <<crm_qi    (k,j,i,iens)
                // // <<"  input_qiil:"<<input_qiil(k_gcm,iens)
                // <<"  state_qi:"<<state_qi(k,j,i,iens)
                <<"  crm_rho_d:" <<crm_rho_d   (k,j,i,iens)
                <<"  crm_pmid:"  <<crm_pmid  (k,j,i,iens)
                <<"  input_pmid:"<<input_pmid(k_gcm,iens)
                <<std::endl;
    // }
  }
  // endrun("stopping for debug");
  //------------------------------------------------------------------------------------------------
  //------------------------------------------------------------------------------------------------
  //------------------------------------------------------------------------------------------------
  // Run the CRM
  real etime_crm = 0;
  while (etime_crm < gcm_dt) {
    if (crm_dt == 0.) { crm_dt = dycore.compute_time_step(coupler); }
    if (etime_crm + crm_dt > gcm_dt) { crm_dt = gcm_dt - etime_crm; }

    // run a PAM time step
    coupler.run_module( "apply_gcm_forcing_tendencies" , modules::apply_gcm_forcing_tendencies );
    coupler.run_module( "sponge_layer"                 , modules::sponge_layer );
    coupler.run_module( "radiation"                    , [&] (pam::PamCoupler &coupler) {rad   .timeStep(coupler);} );
    coupler.run_module( "dycore"                       , [&] (pam::PamCoupler &coupler) {dycore.timeStep(coupler);} );
    // coupler.run_module( "compute_surface_friction"     , modules::compute_surface_friction );
    coupler.run_module( "sgs"                          , [&] (pam::PamCoupler &coupler) {sgs   .timeStep(coupler);} );
    coupler.run_module( "micro"                        , [&] (pam::PamCoupler &coupler) {micro .timeStep(coupler);} );

    pam_radiation_timestep_aggregation(coupler);
    pam_statistics_timestep_aggregation(coupler);

    etime_crm += crm_dt;
  }
  //------------------------------------------------------------------------------------------------
  //------------------------------------------------------------------------------------------------
  //------------------------------------------------------------------------------------------------

  // Compute primary feedback tendencies and copy to GCM
  pam_feedback_compute_crm_feedback_tendencies( coupler, gcm_dt );
  
  // Compute horizontal means of CRM state variables that are not forced
  pam_feedback_compute_crm_mean_state(coupler);

  // Copy the output CRM state from the PAM coupler to the GCM
  pam_state_copy_output_to_gcm(coupler);

  // Copy radiation column quantities to host
  pam_radiation_copy_output_to_gcm(coupler);

  // copy aggregated statistical quantities to host
  pam_statistics_copy_to_host( coupler, gcm_dt );

  //------------------------------------------------------------------------------------------------
  // Finalize and clean up
  micro .finalize(coupler);
  sgs   .finalize(coupler);
  dycore.finalize(coupler);
  rad   .finalize(coupler);
  pam_interface::finalize();
  //------------------------------------------------------------------------------------------------
}

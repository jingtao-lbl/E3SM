#include "catch2/catch.hpp"
#include "physics/rrtmgp/rrtmgp_utils.hpp"
#include "physics/rrtmgp/scream_rrtmgp_interface.hpp"
#include "YAKL.h"
#include "physics/share/physics_constants.hpp"
#include "physics/rrtmgp/share/shr_orb_mod_c2f.hpp"

TEST_CASE("rrtmgp_test_heating") {
    // Initialize YAKL
    if (!yakl::isInitialized()) { yakl::init(); }

    // Test heating rate function by passing simple inputs
    auto dp = real2d("dp", 1, 1);
    auto flux_up = real2d("flux_up", 1, 2);
    auto flux_dn = real2d("flux_dn", 1, 2);
    auto heating = real2d("heating", 1, 1);
    // Simple no-heating test
    // NOTE: parallel_for because we need to do these in a kernel on the device
    parallel_for(1, YAKL_LAMBDA(int /* dummy */) {
        dp(1, 1) = 10;
        flux_up(1, 1) = 1.0;
        flux_up(1, 2) = 1.0;
        flux_dn(1, 1) = 1.0;
        flux_dn(1, 2) = 1.0;
    });
    scream::rrtmgp::compute_heating_rate(flux_up, flux_dn, dp, heating);
    REQUIRE(heating.createHostCopy()(1,1) == 0);

    // Simple net postive heating; net flux into layer should be 1.0
    // NOTE: parallel_for because we need to do these in a kernel on the device
    parallel_for(1, YAKL_LAMBDA(int /* dummy */) {
        flux_up(1, 1) = 1.0;
        flux_up(1, 2) = 1.0;
        flux_dn(1, 1) = 1.5;
        flux_dn(1, 2) = 0.5;
    });
    using physconst = scream::physics::Constants<double>;
    auto g = physconst::gravit; //9.81;
    auto cp_air = physconst::Cpair; //1005.0;
    auto pdel = dp.createHostCopy()(1,1);
    auto heating_ref = 1.0 * g / (cp_air * pdel);
    scream::rrtmgp::compute_heating_rate(flux_up, flux_dn, dp, heating);
    REQUIRE(heating.createHostCopy()(1,1) == heating_ref);

    // Simple net negative heating; net flux into layer should be -1.0
    // NOTE: parallel_for because we need to do these in a kernel on the device
    parallel_for(1, YAKL_LAMBDA(int /* dummy */) {
        flux_up(1,1) = 1.5;
        flux_up(1,2) = 0.5;
        flux_dn(1,1) = 1.0;
        flux_dn(1,2) = 1.0;
    });
    heating_ref = -1.0 * g / (cp_air * pdel);
    scream::rrtmgp::compute_heating_rate(flux_up, flux_dn, dp, heating);
    REQUIRE(heating.createHostCopy()(1,1) == heating_ref);

    // Clean up
    dp.deallocate();
    flux_up.deallocate();
    flux_dn.deallocate();
    heating.deallocate();
    yakl::finalize();
}

TEST_CASE("rrtmgp_test_mixing_ratio_to_cloud_mass") {
    // Initialize YAKL
    if (!yakl::isInitialized()) { yakl::init(); }

    using physconst = scream::physics::Constants<double>;

    // Test mixing ratio to cloud mass function by passing simple inputs
    auto dp = real2d("dp", 1, 1);
    auto mixing_ratio = real2d("mixing_ratio", 1, 1);
    auto cloud_fraction = real2d("cloud_fration", 1, 1);
    auto cloud_mass = real2d("cloud_mass", 1, 1);

    // Test with cell completely filled with cloud
    parallel_for(1, YAKL_LAMBDA(int /* dummy */) {
        dp(1,1) = 10.0;
        mixing_ratio(1,1) = 0.0001;
        cloud_fraction(1,1) = 1.0;
    });
    auto cloud_mass_ref = mixing_ratio.createHostCopy()(1,1) / cloud_fraction.createHostCopy()(1,1) * dp.createHostCopy()(1,1) / physconst::gravit;
    scream::rrtmgp::mixing_ratio_to_cloud_mass(mixing_ratio, cloud_fraction, dp, cloud_mass);
    REQUIRE(cloud_mass.createHostCopy()(1,1) == cloud_mass_ref);

    // Test with no cloud
    parallel_for(1, YAKL_LAMBDA(int /* dummy */) {
        dp(1,1) = 10.0;
        mixing_ratio(1,1) = 0.0;
        cloud_fraction(1,1) = 0.0;
    });
    cloud_mass_ref = 0.0;
    scream::rrtmgp::mixing_ratio_to_cloud_mass(mixing_ratio, cloud_fraction, dp, cloud_mass);
    REQUIRE(cloud_mass.createHostCopy()(1,1) == cloud_mass_ref);
 
     // Test with empty clouds (cloud fraction but with no associated mixing ratio)
     // This case could happen if we use a total cloud fraction, but compute layer
     // cloud mass separately for liquid and ice.
    parallel_for(1, YAKL_LAMBDA(int /* dummy */) {
        dp(1,1) = 10.0;
        mixing_ratio(1,1) = 0.0;
        cloud_fraction(1,1) = 0.1;
    });
    cloud_mass_ref = 0.0;
    scream::rrtmgp::mixing_ratio_to_cloud_mass(mixing_ratio, cloud_fraction, dp, cloud_mass);
    REQUIRE(cloud_mass.createHostCopy()(1,1) == cloud_mass_ref);
 
    // Test with cell half filled with cloud
    parallel_for(1, YAKL_LAMBDA(int /* dummy */) {
        dp(1,1) = 10.0;
        mixing_ratio(1,1) = 0.0001;
        cloud_fraction(1,1) = 0.5;
    });
    cloud_mass_ref = mixing_ratio.createHostCopy()(1,1) / cloud_fraction.createHostCopy()(1,1) * dp.createHostCopy()(1,1) / physconst::gravit;
    scream::rrtmgp::mixing_ratio_to_cloud_mass(mixing_ratio, cloud_fraction, dp, cloud_mass);
    REQUIRE(cloud_mass.createHostCopy()(1,1) == cloud_mass_ref);

    // Clean up
    dp.deallocate();
    mixing_ratio.deallocate();
    cloud_fraction.deallocate();
    cloud_mass.deallocate();
    yakl::finalize();
}

TEST_CASE("rrtmgp_test_limit_to_bounds") {
    // Initialize YAKL
    if (!yakl::isInitialized()) { yakl::init(); }

    // Test limiter function
    auto arr = real2d("arr", 2, 2);
    auto arr_limited = real2d("arr_limited", 2, 2);

    // Setup dummy array
    parallel_for(1, YAKL_LAMBDA(int /* dummy */) {
        arr(1,1) = 1.0;
        arr(1,2) = 2.0;
        arr(2,1) = 3.0;
        arr(2,2) = 4.0;
    });

    // Limit to bounds that contain the data; should be no change in values
    scream::rrtmgp::limit_to_bounds(arr, 0.0, 5.0, arr_limited);
    REQUIRE(arr.createHostCopy()(1,1) == arr_limited.createHostCopy()(1,1));
    REQUIRE(arr.createHostCopy()(1,2) == arr_limited.createHostCopy()(1,2));
    REQUIRE(arr.createHostCopy()(2,1) == arr_limited.createHostCopy()(2,1));
    REQUIRE(arr.createHostCopy()(2,2) == arr_limited.createHostCopy()(2,2));

    // Limit to bounds that do not completely contain the data; should be a change in values!
    scream::rrtmgp::limit_to_bounds(arr, 1.5, 3.5, arr_limited);
    REQUIRE(arr_limited.createHostCopy()(1,1) == 1.5);
    REQUIRE(arr_limited.createHostCopy()(1,2) == 2.0);
    REQUIRE(arr_limited.createHostCopy()(2,1) == 3.0);
    REQUIRE(arr_limited.createHostCopy()(2,2) == 3.5);
    arr.deallocate();
    arr_limited.deallocate();
    yakl::finalize();
}

TEST_CASE("rrtmgp_test_zenith") {

    // Create some dummy data
    int orbital_year = 1990;
    double calday = 1.0000000000000000;
    double eccen_ref = 1.6707719799280658E-002;
    double mvelpp_ref = 4.9344679089867318;
    double lambm0_ref = -3.2503635878519378E-002;
    double obliqr_ref = 0.40912382465788016;
    double delta_ref = -0.40302893695478670;
    double eccf_ref = 1.0342222039093694;
    double lat = -7.7397590528644963E-002;
    double lon = 2.2584340271163548;
    double coszrs_ref = 0.61243613606766745;

    // Test shr_orb_params()
    // Get orbital parameters based on calendar day
    double eccen;
    double obliq;  // obliquity in degrees
    double mvelp;  // moving vernal equinox long of perihelion; degrees?
    double obliqr;
    double lambm0;
    double mvelpp;
    // bool flag_print = false;
    shr_orb_params_c2f(&orbital_year, &eccen, &obliq, &mvelp, 
                     &obliqr, &lambm0, &mvelpp); //, flag_print); // Note fortran code has optional arg
    REQUIRE(eccen == eccen_ref);
    REQUIRE(obliqr == obliqr_ref);
    REQUIRE(mvelpp == mvelpp_ref);
    REQUIRE(lambm0 == lambm0_ref);
    REQUIRE(mvelpp == mvelpp_ref);

    // Test shr_orb_decl()
    double delta;
    double eccf;
    shr_orb_decl_c2f(calday, eccen, mvelpp, lambm0,
                   obliqr, &delta, &eccf);
    REQUIRE(delta == delta_ref);
    REQUIRE(eccf  == eccf_ref );

    double dt_avg = 0.; //3600.0000000000000;
    double coszrs = shr_orb_cosz_c2f(calday, lat, lon, delta, dt_avg);
    REQUIRE(std::abs(coszrs-coszrs_ref)<1e-14);

    // Another case, this time WITH dt_avg flag:
    calday = 1.0833333333333333;
    eccen = 1.6707719799280658E-002;
    mvelpp = 4.9344679089867318;
    lambm0 = -3.2503635878519378E-002;
    obliqr = 0.40912382465788016;
    delta = -0.40292121709083456;
    eccf = 1.0342248931660425;
    lat = -1.0724153591027763;
    lon = 4.5284876076962712;
    dt_avg = 3600.0000000000000;
    coszrs_ref = 0.14559973262047626;
    coszrs = shr_orb_cosz_c2f(calday, lat, lon, delta, dt_avg);
    REQUIRE(std::abs(coszrs-coszrs_ref)<1e-14);

}

TEST_CASE("rrtmgp_test_compute_broadband_surface_flux") {
    using namespace ekat::logger;
    using logger_t = Logger<LogNoFile,LogRootRank>;

    ekat::Comm comm(MPI_COMM_WORLD);
    auto logger = std::make_shared<logger_t>("",LogLevel::info,comm);

    // Initialize YAKL
    if (!yakl::isInitialized()) { yakl::init(); }

    // Create arrays
    const int ncol = 1;
    const int nlay = 1;
    const int nbnd = 14;
    const int kbot = nlay + 1;
    auto sfc_flux_dir_nir = real1d("sfc_flux_dir_nir", ncol);
    auto sfc_flux_dir_vis = real1d("sfc_flux_dir_vis", ncol);
    auto sfc_flux_dif_nir = real1d("sfc_flux_dif_nir", ncol);
    auto sfc_flux_dif_vis = real1d("sfc_flux_dif_vis", ncol);

    // Need to initialize RRTMGP with dummy gases
    logger->info("Init gases...\n");
    GasConcs gas_concs;
    int ngas = 8;
    string1d gas_names("gas_names",ngas);
    gas_names(1) = std::string("h2o");
    gas_names(2) = std::string("co2");
    gas_names(3) = std::string("o3" );
    gas_names(4) = std::string("n2o");
    gas_names(5) = std::string("co" );
    gas_names(6) = std::string("ch4");
    gas_names(7) = std::string("o2" );
    gas_names(8) = std::string("n2" );
    gas_concs.init(gas_names,ncol,nlay);
    logger->info("Init RRTMGP...\n");
    scream::rrtmgp::rrtmgp_initialize(gas_concs,logger);

    // Create simple test cases; We expect, given the input data, that band 10
    // will straddle the NIR and VIS, bands 1-9 will be purely NIR, and bands 11-14
    // will be purely VIS. The implementation in EAMF90 was hard-coded with this
    // band information, but our implementation of compute_broadband_surface_fluxes
    // actually checks the wavenumber limits. These tests will mostly check to make
    // sure our implementation of that is doing what we think it is.

    // ---------------------------------
    // Test case: flux only in straddled band
    auto sw_bnd_flux_dir = real3d("sw_bnd_flux_dir", ncol, nlay+1, nbnd);
    auto sw_bnd_flux_dif = real3d("sw_bnd_flux_dif", ncol, nlay+1, nbnd);
    logger->info("Populate band-resolved 3d fluxes for test case with only transition band flux...\n");
    parallel_for(Bounds<3>(nbnd,nlay+1,ncol), YAKL_LAMBDA(int ibnd, int ilay, int icol) {
        if (ibnd < 10) {
            sw_bnd_flux_dir(icol,ilay,ibnd) = 0;
            sw_bnd_flux_dif(icol,ilay,ibnd) = 0;
        } else if (ibnd == 10) {
            sw_bnd_flux_dir(icol,ilay,ibnd) = 1;
            sw_bnd_flux_dif(icol,ilay,ibnd) = 1;
        } else {
            sw_bnd_flux_dir(icol,ilay,ibnd) = 0;
            sw_bnd_flux_dif(icol,ilay,ibnd) = 0;
        }
    });
    // Compute surface fluxes
    logger->info("Compute broadband surface fluxes...\n");
    scream::rrtmgp::compute_broadband_surface_fluxes(
        ncol, kbot, nbnd,
        sw_bnd_flux_dir, sw_bnd_flux_dif,
        sfc_flux_dir_vis, sfc_flux_dir_nir,
        sfc_flux_dif_vis, sfc_flux_dif_nir
    );
    // Check computed surface fluxes
    logger->info("Check computed fluxes...\n");
    const double tol = 1e-10;  // tolerance on floating point inequality for assertions
    REQUIRE(std::abs(sfc_flux_dir_nir.createHostCopy()(1) - 0.5) < tol);
    REQUIRE(std::abs(sfc_flux_dir_vis.createHostCopy()(1) - 0.5) < tol);
    REQUIRE(std::abs(sfc_flux_dif_nir.createHostCopy()(1) - 0.5) < tol);
    REQUIRE(std::abs(sfc_flux_dif_vis.createHostCopy()(1) - 0.5) < tol);
    // ---------------------------------

    // ---------------------------------
    // Test case, only flux in NIR bands
    logger->info("Populate band-resolved 3d fluxes for test case with only NIR flux...\n");
    parallel_for(Bounds<3>(nbnd,nlay+1,ncol), YAKL_LAMBDA(int ibnd, int ilay, int icol) {
        if (ibnd < 10) {
            sw_bnd_flux_dir(icol,ilay,ibnd) = 1;
            sw_bnd_flux_dif(icol,ilay,ibnd) = 1;
        } else if (ibnd == 10) {
            sw_bnd_flux_dir(icol,ilay,ibnd) = 0;
            sw_bnd_flux_dif(icol,ilay,ibnd) = 0;
        } else {
            sw_bnd_flux_dir(icol,ilay,ibnd) = 0;
            sw_bnd_flux_dif(icol,ilay,ibnd) = 0;
        }
    });
    // Compute surface fluxes
    logger->info("Compute broadband surface fluxes...\n");
    scream::rrtmgp::compute_broadband_surface_fluxes(
        ncol, kbot, nbnd,
        sw_bnd_flux_dir, sw_bnd_flux_dif,
        sfc_flux_dir_vis, sfc_flux_dir_nir,
        sfc_flux_dif_vis, sfc_flux_dif_nir
    );
    // Check computed surface fluxes
    logger->info("Check computed fluxes...\n");
    REQUIRE(std::abs(sfc_flux_dir_nir.createHostCopy()(1) - 9.0) < tol);
    REQUIRE(std::abs(sfc_flux_dir_vis.createHostCopy()(1) - 0.0) < tol);
    REQUIRE(std::abs(sfc_flux_dif_nir.createHostCopy()(1) - 9.0) < tol);
    REQUIRE(std::abs(sfc_flux_dif_vis.createHostCopy()(1) - 0.0) < tol);
    // ---------------------------------
 
    // ---------------------------------
    // Test case, only flux in VIS bands
    logger->info("Populate band-resolved 3d fluxes for test case with only VIS/UV flux...\n");
    parallel_for(Bounds<3>(nbnd,nlay+1,ncol), YAKL_LAMBDA(int ibnd, int ilay, int icol) {
        if (ibnd < 10) {
            sw_bnd_flux_dir(icol,ilay,ibnd) = 0;
            sw_bnd_flux_dif(icol,ilay,ibnd) = 0;
        } else if (ibnd == 10) {
            sw_bnd_flux_dir(icol,ilay,ibnd) = 0;
            sw_bnd_flux_dif(icol,ilay,ibnd) = 0;
        } else {
            sw_bnd_flux_dir(icol,ilay,ibnd) = 1;
            sw_bnd_flux_dif(icol,ilay,ibnd) = 1;
        }
    });
    // Compute surface fluxes
    logger->info("Compute broadband surface fluxes...\n");
    scream::rrtmgp::compute_broadband_surface_fluxes(
        ncol, kbot, nbnd,
        sw_bnd_flux_dir, sw_bnd_flux_dif,
        sfc_flux_dir_vis, sfc_flux_dir_nir,
        sfc_flux_dif_vis, sfc_flux_dif_nir
    );
    // Check computed surface fluxes
    logger->info("Check computed fluxes...\n");
    REQUIRE(std::abs(sfc_flux_dir_nir.createHostCopy()(1) - 0.0) < tol);
    REQUIRE(std::abs(sfc_flux_dir_vis.createHostCopy()(1) - 4.0) < tol);
    REQUIRE(std::abs(sfc_flux_dif_nir.createHostCopy()(1) - 0.0) < tol);
    REQUIRE(std::abs(sfc_flux_dif_vis.createHostCopy()(1) - 4.0) < tol);
    // ---------------------------------

    // ---------------------------------
    // Test case, only flux in all bands
    logger->info("Populate band-resolved 3d fluxes for test with non-zero flux in all bands...\n");
    parallel_for(Bounds<3>(nbnd,nlay+1,ncol), YAKL_LAMBDA(int ibnd, int ilay, int icol) {
        if (ibnd < 10) {
            sw_bnd_flux_dir(icol,ilay,ibnd) = 1.0;
            sw_bnd_flux_dif(icol,ilay,ibnd) = 2.0;
        } else if (ibnd == 10) {
            sw_bnd_flux_dir(icol,ilay,ibnd) = 3.0;
            sw_bnd_flux_dif(icol,ilay,ibnd) = 4.0;
        } else {
            sw_bnd_flux_dir(icol,ilay,ibnd) = 5.0;
            sw_bnd_flux_dif(icol,ilay,ibnd) = 6.0;
        }
    });
    // Compute surface fluxes
    logger->info("Compute broadband surface fluxes...\n");
    scream::rrtmgp::compute_broadband_surface_fluxes(
        ncol, kbot, nbnd,
        sw_bnd_flux_dir, sw_bnd_flux_dif,
        sfc_flux_dir_vis, sfc_flux_dir_nir,
        sfc_flux_dif_vis, sfc_flux_dif_nir
    );
    // Check computed surface fluxes
    logger->info("Check computed fluxes...\n");
    REQUIRE(std::abs(sfc_flux_dir_nir.createHostCopy()(1) - 10.5) < tol);
    REQUIRE(std::abs(sfc_flux_dir_vis.createHostCopy()(1) - 21.5) < tol);
    REQUIRE(std::abs(sfc_flux_dif_nir.createHostCopy()(1) - 20.0) < tol);
    REQUIRE(std::abs(sfc_flux_dif_vis.createHostCopy()(1) - 26.0) < tol);
    // ---------------------------------

    // Finalize YAKL
    logger->info("Free memory...\n");
    scream::rrtmgp::rrtmgp_finalize();
    gas_concs.reset();
    gas_names.deallocate();
    sw_bnd_flux_dir.deallocate();
    sw_bnd_flux_dif.deallocate();
    sfc_flux_dir_nir.deallocate();
    sfc_flux_dir_vis.deallocate();
    sfc_flux_dif_nir.deallocate();
    sfc_flux_dif_vis.deallocate();
    if (yakl::isInitialized()) { yakl::finalize(); }
}

TEST_CASE("rrtmgp_test_radiation_do") {
    // If we specify rad every step, radiation_do should always be true
    REQUIRE(scream::rrtmgp::radiation_do(1, 0) == true);
    REQUIRE(scream::rrtmgp::radiation_do(1, 1) == true);
    REQUIRE(scream::rrtmgp::radiation_do(1, 2) == true);

    // Test cases where we want rad called every other step
    REQUIRE(scream::rrtmgp::radiation_do(2, 0) == true);
    REQUIRE(scream::rrtmgp::radiation_do(2, 1) == false);
    REQUIRE(scream::rrtmgp::radiation_do(2, 2) == true);
    REQUIRE(scream::rrtmgp::radiation_do(2, 3) == false);

    // Test cases where we want rad every third step
    REQUIRE(scream::rrtmgp::radiation_do(3, 0) == true);
    REQUIRE(scream::rrtmgp::radiation_do(3, 1) == false);
    REQUIRE(scream::rrtmgp::radiation_do(3, 2) == false);
    REQUIRE(scream::rrtmgp::radiation_do(3, 3) == true);
    REQUIRE(scream::rrtmgp::radiation_do(3, 4) == false);
    REQUIRE(scream::rrtmgp::radiation_do(3, 5) == false);
    REQUIRE(scream::rrtmgp::radiation_do(3, 6) == true);
}

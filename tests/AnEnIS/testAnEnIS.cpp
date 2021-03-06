/*
 * File:   testAnEnIS.cpp
 * Author: Weiming Hu <weiming@psu.edu>
 *
 * Created on Aug 4, 2018, 4:09:20 PM
 */

#include <numeric>

#include "testAnEnIS.h"
#include "boost/bimap/bimap.hpp"
#include "boost/assign/list_of.hpp"
#include "boost/assign/list_inserter.hpp"
#include "ForecastsPointer.h"
#include "ObservationsPointer.h"

#if defined(_OPENMP)
#include <omp.h>
#endif

using namespace std;
using namespace boost;
using namespace boost::bimaps;

CPPUNIT_TEST_SUITE_REGISTRATION(testAnEnIS);

testAnEnIS::testAnEnIS() {
}

testAnEnIS::~testAnEnIS() {
}

void
testAnEnIS::setUp() {

    // Make sure member variables are empty so that they
    // will be further prepared in other setUp* functions.
    //
    CPPUNIT_ASSERT(parameters_.size() == 0);
    CPPUNIT_ASSERT(stations_.size() == 0);
    CPPUNIT_ASSERT(fcst_times_.size() == 0);
    CPPUNIT_ASSERT(obs_times_.size() == 0);
    CPPUNIT_ASSERT(flts_.size() == 0);
    return;
}

void
testAnEnIS::tearDown() {
    parameters_.clear();
    stations_.clear();
    fcst_times_.clear();
    obs_times_.clear();
    flts_.clear();
    return;
}

void
testAnEnIS::setUpSds() {

    /*
     * This function sets up the following:
     * 
     *  - 3 parameters
     *  - 2 stations
     *  - 10 forecast time stamps
     *  - 2 FLTs
     */

    setUp();

    // Manually create 3 parameters
    assign::push_back(parameters_.left)
            (0, Parameter("par_1")) // Linear parameter
            (1, Parameter("par_2")) // Parameter that will be assigned with 0 weight
            (2, Parameter("par_3", true)); // Circular parameter

    weights_ = {1, 0, 1};

    // Manually create 2 stations
    assign::push_back(stations_.left)
            (0, Station(50, 20))
            (1, Station(10, 10, "station_2"));

    // Manually create 10 times
    for (size_t i = 0; i < 10; ++i) {
        fcst_times_.push_back(Time(i * 100));
    }

    // Manually create 2 FLTs
    assign::push_back(flts_.left)(0, Time(0))(1, Time(50));
    return;
}

void
testAnEnIS::tearDownSds() {
    tearDown();
    return;
}

void
testAnEnIS::setUpCompute() {

    /*
     * This function sets up the following dimensions:
     * 
     *  - 3 parameters
     *  - 2 stations
     *  - 20 time stamps
     *  - 3 FLTs (forecast only)
     */

    setUp();

    // Manually create 5 parameters
    assign::push_back(parameters_.left)
            (0, Parameter("par_1")) // Linear parameter
            (1, Parameter("par_2")) // Parameter with 0 weight
            (2, Parameter("par_3", true)); // Circular parameter

    weights_ = {1, 0, 1};

    // Manually create 2 stations
    assign::push_back(stations_.left)
            (0, Station(50, 20))
            (1, Station(10, 10, "station_2"));

    // Manually create 20 forecast times
    for (size_t i = 0; i < 20; ++i) {
        fcst_times_.push_back(Time(i * 100));
    }

    // Manually create 3 FLTs
    assign::push_back(flts_.left)(0, Time(0))(1, Time(50))(0, Time(100));

    // Manually create 50 observation times
    for (size_t i = 0; i < 50; ++i) {
        obs_times_.push_back(i * 50);
    }

    return;
}

void
testAnEnIS::tearDownCompute() {
    tearDown();
    return;
}

void
testAnEnIS::testOpenMP_() {

    /*
     * Test whether OpenMP is supported and how many threads are created.
     */

#if defined(_OPENMP)
    cout << "OpenMP is supported." << endl;

    int num_threads;

#pragma omp parallel for default(none) schedule(static) shared(num_threads)
    for (size_t i = 0; i < 100; i++) {
        double tmp = 42 * 3.14;
        if (0 == omp_get_thread_num()) num_threads = omp_get_num_threads();
        tmp *= 2;
    }

    cout << "There are ";
    if (num_threads < 50) cout << num_threads;
    else cout << "> 50";
    cout << " threads created.";

#else
    cout << "OpenMP is NOT supported." << endl;
#endif

}

void
testAnEnIS::testMultiAnEn_() {

    setUpCompute();

    ForecastsPointer fcsts(parameters_, stations_, fcst_times_, flts_);
    ObservationsPointer obs(parameters_, stations_, obs_times_);

    // Assign random forecast values
    randomizeForecasts_(fcsts, 0.4);

    // Assign random observation values
    randomizeObservations_(obs, 0.2);

    /*
     * Carry out leave one out tests for 3 days
     */
    Config config;
    config.num_analogs = 10;
    config.operation = true;
    config.obs_var_index = 1;
    config.save_analogs_time_index = true;
    config.max_par_nan = fcsts.getParameters().size();
    config.max_flt_nan = fcsts.getFLTs().size();

    // Define test indices
    vector<size_t> fcsts_test_index = {17, 18, 19};
    vector<size_t> fcsts_search_index(17);
    iota(fcsts_search_index.begin(), fcsts_search_index.end(), 0);

    // Compute analogs
    AnEnIS anen(config);
    anen.compute(fcsts, obs, fcsts_test_index, fcsts_search_index);

    const auto & analogs = anen.analogs_value();

    // Get values using Functions
    Array4DPointer analogs_queried;
    const auto & analogs_time_index = anen.analogs_time_index();
    Functions::toValues(analogs_queried, config.obs_var_index, analogs_time_index, obs);

    // Compare
    CPPUNIT_ASSERT(analogs.shape()[0] == analogs_queried.shape()[0]);
    CPPUNIT_ASSERT(analogs.shape()[1] == analogs_queried.shape()[1]);
    CPPUNIT_ASSERT(analogs.shape()[2] == analogs_queried.shape()[2]);
    CPPUNIT_ASSERT(analogs.shape()[3] == analogs_queried.shape()[3]);
    for (size_t i = 0; i < analogs.num_elements(); ++i) {
        CPPUNIT_ASSERT(analogs.getValuesPtr()[i] == analogs_queried.getValuesPtr()[i]);
    }

}

void
testAnEnIS::testFixedLengthSds_() {

    /*
     * This function compares the standard deviation calculation with R.
     */
    setUpSds();

    ForecastsPointer forecasts(parameters_, stations_, fcst_times_, flts_);
    double *ptr = forecasts.getValuesPtr();
    vector<double> values = {
        NAN, -17.35, NAN, 1.92, NAN, 39.82, 6.95, -3.42, -39.85,
        -11.44, -8.55, 1.27, 6.01, 38.61, 37.24, -13.4, -5.81, -10.32, 25.49,
        -42.99, -3.4, -24.89, 8.75, 29.19, -32.71, 16.56, -14.38, -46.59,
        46.89, 27.76, 1.45, 35.77, 41.08, 2.96, 23.83, 48.35, 15.22, -40.96,
        -1.89, -24.06, 25.58, 22.18, -14.34, -44.29, 35.93, 24.38, 33.92, 43.13,
        -23.54, -10.65, 16.86, -20.4, -27.47, -24.48, 12.12, 5.73, -24.05, 14.8,
        5.81, -25.34, -7.15, 43.57, -2.19, 24.7, 10.51, 32.83, 24.15, -17.91,
        45.34, 15.51, 10.94, 23.9, 11.03, -38.01, 48.98, 10.75, 26.88, 41.81,
        3.62, 12.91, -32.52, -16.61, -6.76, -46.59, -39.25, -12.19, 30, -30.55,
        -46.86, 47.09, -49.08, 21.64, 36.05, 49.57, 35.4, -32.09, -45.59, 23.47,
        -4.66, -6.93, -22.84, 20.58, 18.63, 29.66, 13.49, -38.86, 19.04, 20.11,
        19.6, 44.91, 40.41, 21.06, -47.33, -28.54, 7.03, 23.87, 17.4, 41.97,
        11.82, 24.89
    };

    for (size_t i = 0; i < values.size(); ++i) ptr[i] = values[i];

    vector<size_t> times_fixed_index;
    for (size_t i = 0; i < fcst_times_.size(); ++i) {
        times_fixed_index.push_back(i);
    }

    operation_ = false;
    computeSds_(forecasts, times_fixed_index);

    // Answers from R
    vector<double> answers = {
        19.181, 28.3874, 21.045, 29.6220, NAN, NAN, NAN, NAN,
        27.6836, 24.8016, 26.5622, 31.8882
    };

    // Check for correctness
    size_t pos = 0;
    for (size_t i = 0; i < sds_.shape()[0]; ++i) {
        for (size_t j = 0; j < sds_.shape()[1]; ++j) {
            for (size_t m = 0; m < sds_.shape()[2]; ++m) {
                for (size_t n = 0; n < sds_.shape()[3]; ++n, ++pos) {
                    cout << sds_.getValue(i, j, m, n) << " " << answers[pos] << endl;

                    if (std::isnan(answers[pos])) {
                        CPPUNIT_ASSERT(std::isnan(sds_.getValue(i, j, m, n)));
                    } else {
                        CPPUNIT_ASSERT(abs(sds_.getValue(i, j, m, n) - answers[pos]) < 1e-4);
                    }
                }
            }
        }
    }

    tearDownSds();
}

void
testAnEnIS::compareOperationalSds_() {

    /*
     * This function compares the results of standard deviation calculation
     * between fixed-length calculation and the operational calculation.
     */

    setUpSds();

#if defined(_OPENMP)
    // Since the test case is small, creating threads actually slows it down
    omp_set_num_threads(1);
#endif

    /*
     * Generate random numbers with a series of probabilities of being NAN
     */
    for (double nan_prob :{0.0, 0.3, 0.5, 0.7, 0.9, 1.0}) {

        cout << "Test with an NAN probability of " << nan_prob << " ..." << endl;

        ForecastsPointer forecasts(parameters_, stations_, fcst_times_, flts_);
        randomizeForecasts_(forecasts, nan_prob, 2);

        /*
         * Calculate the running standard deviation for different fixed length
         */
        for (size_t num_fixed_indices :{2, 4, 6, 8}) {
            cout << "\t Test with a fixed length of "
                    << num_fixed_indices << " ..." << endl;

            vector<size_t> times_fixed_index, times_accum_index;

            for (size_t i = 0; i < num_fixed_indices; ++i) {
                times_fixed_index.push_back(i);
            }

            for (size_t i = num_fixed_indices; i < fcst_times_.size(); ++i) {
                times_accum_index.push_back(i);
            }

            operation_ = true;
            computeSds_(forecasts, times_fixed_index, times_accum_index);

            // Save the running calculation result
            Array4DPointer sds_running = sds_;

            /*
             * Manually calculate the fixed-length standard deviation for each running
             * time and compare the results.
             */
            operation_ = false;
            for (auto time_accum_index : times_accum_index) {

                // Manually set up standard deviation calculation
                vector<size_t> times_fixed_index_manual;
                for (size_t i = 0; i < time_accum_index; ++i) times_fixed_index_manual.push_back(i);
                computeSds_(forecasts, times_fixed_index_manual);

                // Compare results
                size_t running_sd_time_index = time_accum_index - num_fixed_indices;

                for (size_t par_i = 0; par_i < parameters_.size(); ++par_i) {
                    for (size_t sta_i = 0; sta_i < stations_.size(); ++sta_i) {
                        for (size_t flt_i = 0; flt_i < flts_.size(); ++flt_i) {

                            double sd_fixed = sds_.getValue(par_i, sta_i, flt_i, 0);
                            double sd_running = sds_running.getValue(par_i, sta_i, flt_i, running_sd_time_index);

                            if (std::isnan(sd_fixed)) {
                                CPPUNIT_ASSERT(std::isnan(sd_running));
                            } else {
                                CPPUNIT_ASSERT(sd_fixed == sd_running);
                            }
                        }
                    }
                }
            }
        }
    }

    tearDownSds();
    return;
}

void
testAnEnIS::compareComputeLeaveOneOut_() {

    /*
     * Test checking of overlapping times during analog calculation
     */

    setUpCompute();

    /*
     * Run tests for a series of NAN probabilities
     */
    for (double nan_prob :{0.0, 0.2, 0.4, 0.6, 0.8, 0.9}) {


        /*
         * Initialize forecasts and observations
         */
        cout << "Test with an NAN probability of " << nan_prob << " ..." << endl;

        ForecastsPointer fcsts(parameters_, stations_, fcst_times_, flts_);
        ObservationsPointer obs(parameters_, stations_, obs_times_);

        // Assign random forecast values
        randomizeForecasts_(fcsts, nan_prob);

        // Assign random observation values
        randomizeObservations_(obs, nan_prob);

        /*
         * Carry out leave one out tests for 3 days
         */
        Config config;
        config.num_analogs = 10;
        config.operation = false;
        config.prevent_search_future = false;
        config.save_sims = true;
        config.verbose = Verbose::Warning;
        config.obs_var_index = 1;
        config.quick_sort = false;
        config.save_sims_time_index = true;
        config.save_analogs_time_index = true;
        config.max_par_nan = fcsts.getParameters().size();
        config.max_flt_nan = fcsts.getFLTs().size();
        config.flt_radius = 1;

        // Define test indices
        vector<size_t> fcsts_test_index = {0, 5, 10, 15, 19};
        vector<size_t> fcsts_search_index(fcsts.getTimes().size());
        iota(fcsts_search_index.begin(), fcsts_search_index.end(), 0);

        // Compute analogs
        AnEnIS anen(config);
        anen.compute(fcsts, obs, fcsts_test_index, fcsts_search_index);

        // Copy results
        Array4DPointer my_analogs = anen.analogs_value();
        Array4DPointer my_analogs_index = anen.analogs_time_index();
        Array4DPointer my_sims = anen.sims_metric();
        Array4DPointer my_sims_index = anen.sims_time_index();

        /*
         * Carry out manual calculation for each test day
         */
        for (size_t test_i = 0; test_i < fcsts_test_index.size(); ++test_i) {

            // Remove the test index from the search indices
            size_t fcst_test_index = fcsts_test_index[test_i];
            vector<size_t> manual_search_index = fcsts_search_index;
            vector<size_t> test_index{fcst_test_index};

            // Calculate analogs for the manual setting
            anen.compute(fcsts, obs, test_index, manual_search_index);

            // Get references to results
            const Array4DPointer & manual_analogs = anen.analogs_value();
            const Array4DPointer & manual_analogs_index = anen.analogs_time_index();
            const Array4DPointer & manual_sims = anen.sims_metric();
            const Array4DPointer & manual_sims_index = anen.sims_time_index();

            for (size_t i = 0; i < my_analogs.shape()[0]; ++i) {
                for (size_t m = 0; m < my_analogs.shape()[2]; ++m) {
                    for (size_t n = 0; n < my_analogs.shape()[3]; ++n) {

                        if (std::isnan(my_analogs.getValue(i, test_i, m, n))) {
                            CPPUNIT_ASSERT(manual_analogs.getValue(i, 0, m, n));
                        } else {
                            CPPUNIT_ASSERT(my_analogs.getValue(i, test_i, m, n) ==
                                    manual_analogs.getValue(i, 0, m, n));
                        }

                        if (std::isnan(my_analogs_index.getValue(i, test_i, m, n))) {
                            CPPUNIT_ASSERT(manual_analogs_index.getValue(i, 0, m, n));
                        } else {
                            CPPUNIT_ASSERT(my_analogs_index.getValue(i, test_i, m, n) ==
                                    manual_analogs_index.getValue(i, 0, m, n));
                        }

                        if (std::isnan(my_sims.getValue(i, test_i, m, n))) {
                            CPPUNIT_ASSERT(manual_sims.getValue(i, 0, m, n));
                        } else {
                            CPPUNIT_ASSERT(my_sims.getValue(i, test_i, m, n) ==
                                    manual_sims.getValue(i, 0, m, n));
                        }

                        if (std::isnan(my_sims_index.getValue(i, test_i, m, n))) {
                            CPPUNIT_ASSERT(manual_sims_index.getValue(i, 0, m, n));
                        } else {
                            CPPUNIT_ASSERT(my_sims_index.getValue(i, test_i, m, n) ==
                                    manual_sims_index.getValue(i, 0, m, n));
                        }
                    }
                }
            }
        }
    }

    tearDownCompute();

    return;
}

void
testAnEnIS::compareComputeOperational_() {

    /*
     * Test the operation mode
     */

    setUpCompute();

    ForecastsPointer fcsts(parameters_, stations_, fcst_times_, flts_);
    ObservationsPointer obs(parameters_, stations_, obs_times_);

    // Assign random forecast values
    randomizeForecasts_(fcsts, 0);

    // Assign random observation values
    randomizeObservations_(obs, 0);

    /*
     * Run the AnEnIS generation in operation
     */

    // Create configuration
    Config config;
    config.quick_sort = false;
    config.num_analogs = 3;
    config.operation = true;
    config.save_analogs = true;
    config.save_analogs_time_index = true;
    config.save_sims = true;
    config.save_sims_time_index = true;

    // Define test and search days
    vector<size_t> fcsts_test_index = {16, 17, 18, 19};
    vector<size_t> fcsts_search_index(16);
    iota(fcsts_search_index.begin(), fcsts_search_index.end(), 0);
    cout << "AnEnIS generation in operation for tests index: "
            << Functions::format(fcsts_test_index) << endl;

    // Run AnEnIS generation
    AnEnIS anen(config);
    anen.compute(fcsts, obs, fcsts_test_index, fcsts_search_index);

    // Copy results
    Array4DPointer my_analogs = anen.analogs_value();
    Array4DPointer my_analogs_index = anen.analogs_time_index();
    Array4DPointer my_sims = anen.sims_metric();
    Array4DPointer my_sims_index = anen.sims_time_index();

    // Since we increase the number of analogs, the number of similarity
    // should be increased automatically to be consistent with the number
    // of analogs.
    //
    CPPUNIT_ASSERT(my_analogs.shape()[3] == my_sims.shape()[3]);

    /*
     * For each of the test day, run AnEnIS generation manually
     */
    config.operation = false;

    for (size_t test_index :{16, 17, 18, 19}) {
        cout << "Compare the operational AnEnIS with manual AnEnIS on test index " << test_index << endl;

        // Define test and search days
        vector<size_t> fcsts_test_index_manual = {test_index};
        vector<size_t> fcsts_search_index_manual(test_index);
        iota(fcsts_search_index_manual.begin(), fcsts_search_index_manual.end(), 0);

        // Run AnEnIS generation
        AnEnIS anen(config);
        anen.compute(fcsts, obs, fcsts_test_index_manual, fcsts_search_index_manual);

        // Copy results
        Array4DPointer manual_analogs = anen.analogs_value();
        Array4DPointer manual_analogs_index = anen.analogs_time_index();
        Array4DPointer manual_sims = anen.sims_metric();
        Array4DPointer manual_sims_index = anen.sims_time_index();

        // Compare the results
        for (size_t station_i = 0; station_i < stations_.size(); ++station_i) {
            for (size_t flt_i = 0; flt_i < flts_.size(); ++flt_i) {
                for (size_t member_i = 0; member_i < config.num_analogs; ++member_i) {

                    if (std::isnan(my_analogs.getValue(station_i, test_index - 16, flt_i, member_i))) {
                        CPPUNIT_ASSERT(manual_analogs.getValue(station_i, 0, flt_i, member_i));
                    } else {
                        CPPUNIT_ASSERT(my_analogs.getValue(station_i, test_index - 16, flt_i, member_i) ==
                                manual_analogs.getValue(station_i, 0, flt_i, member_i));
                    }

                    if (std::isnan(my_analogs_index.getValue(station_i, test_index - 16, flt_i, member_i))) {
                        CPPUNIT_ASSERT(manual_analogs_index.getValue(station_i, 0, flt_i, member_i));
                    } else {
                        CPPUNIT_ASSERT(my_analogs_index.getValue(station_i, test_index - 16, flt_i, member_i) ==
                                manual_analogs_index.getValue(station_i, 0, flt_i, member_i));
                    }

                    if (std::isnan(my_sims.getValue(station_i, test_index - 16, flt_i, member_i))) {
                        CPPUNIT_ASSERT(manual_sims.getValue(station_i, 0, flt_i, member_i));
                    } else {
                        CPPUNIT_ASSERT(my_sims.getValue(station_i, test_index - 16, flt_i, member_i) ==
                                manual_sims.getValue(station_i, 0, flt_i, member_i));
                    }

                    if (std::isnan(my_sims_index.getValue(station_i, test_index - 16, flt_i, member_i))) {
                        CPPUNIT_ASSERT(manual_sims_index.getValue(station_i, 0, flt_i, member_i));
                    } else {
                        CPPUNIT_ASSERT(my_sims_index.getValue(station_i, test_index - 16, flt_i, member_i) ==
                                manual_sims_index.getValue(station_i, 0, flt_i, member_i));
                    }

                }
            }
        }
    }

    tearDownCompute();

    return;
}

void
testAnEnIS::randomizeForecasts_(Forecasts & fcsts,
        double nan_prob, size_t min_valid_count) const {

    for (size_t par_i = 0; par_i < fcsts.getParameters().size(); ++par_i) {
        for (size_t sta_i = 0; sta_i < fcsts.getStations().size(); ++sta_i) {
            for (size_t flt_i = 0; flt_i < fcsts.getFLTs().size(); ++flt_i) {

                for (size_t time_i = 0; time_i < 2; ++time_i) {
                    fcsts.setValue((rand() % 10000) / 100.0,
                            par_i, sta_i, time_i, flt_i);
                }

                for (size_t time_i = min_valid_count;
                        time_i < fcsts.getTimes().size(); ++time_i) {

                    double prob = rand() / double(RAND_MAX);
                    if (prob < nan_prob) {
                        fcsts.setValue(NAN, par_i, sta_i, time_i, flt_i);
                    } else {

                        fcsts.setValue((rand() % 10000) / 100.0,
                                par_i, sta_i, time_i, flt_i);
                    }
                }
            }
        }
    }
    return;
}

void
testAnEnIS::randomizeObservations_(Observations & obs, double nan_prob) const {

    for (size_t par_i = 0; par_i < obs.getParameters().size(); ++par_i) {
        for (size_t sta_i = 0; sta_i < obs.getStations().size(); ++sta_i) {
            for (size_t time_i = 0; time_i < obs.getTimes().size(); ++time_i) {

                double prob = rand() / double(RAND_MAX);
                if (prob < nan_prob) {
                    obs.setValue(NAN, par_i, sta_i, time_i);
                } else {
                    obs.setValue((rand() % 10000) / 100.0,
                            par_i, sta_i, time_i);
                }
            }
        }
    }
    return;
}

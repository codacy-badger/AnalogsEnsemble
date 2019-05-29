/* 
 * File:   AnEn.cpp
 * Author: Weiming Hu <weiming@psu.edu>
 * 
 * Created on July 23, 2018, 10:56 PM
 */

#include "AnEn.h"
#include "colorTexts.h"
#include <boost/numeric/ublas/io.hpp>

#define _USE_MATH_DEFINES
#include <cmath>
#include <ostream>

#if defined(_OPENMP)
#include <omp.h>
#endif

using namespace std;
using simMethod = AnEn::simMethod;

const double AnEn::_FILL_VALUE = NAN;
const size_t AnEn::_SERIAL_LENGTH_LIMIT = 500;

AnEn::AnEn() {
}

AnEn::AnEn(int verbose) :
verbose_(verbose) {
}

AnEn::~AnEn() {
}

errorType
AnEn::computeSearchStations(
        const anenSta::Stations & test_stations,
        const anenSta::Stations & search_stations,
        AnEn::SearchStationMatrix & i_search_stations_ref,
        size_t max_num_search_stations,
        double distance, size_t num_nearest_stations,
        vector<size_t> test_station_tags,
        vector<size_t> search_station_tags) const {

    // Check max number of search stations. If the number of nearest neighbor
    // is fixed, this will also be the maximum number of search station.
    //
    if (num_nearest_stations != 0) {
        max_num_search_stations = num_nearest_stations;
    }
    
    // Get the test station container by insertion order
    const auto & test_stations_by_insert = test_stations.get<anenSta::by_insert>();
    const auto & search_stations_by_insert = search_stations.get<anenSta::by_insert>();

    // Make sure the tag vectors are initialized correctly
    if (search_station_tags.size() == 0) {
        // If search station tags are not specified, it is assumed that
        // all search stations have the same tag.
        //
        search_station_tags.resize(search_stations.size(), 0);
    }

    if (test_station_tags.size() == 0) {
        // If test station tags are not specified, I need to check whether
        // search and test stations are literally the same before I
        // reuse the tags from search stations on test stations. Otherwise,
        // all test stations are considered the same tag as search stations.
        //
        bool same_stations = true;
        
        if (test_stations.size() == search_stations.size()) {
            
            for (size_t i = 0; i < search_stations.size(); i++) {
                if (!test_stations_by_insert[i].literalCompare(search_stations_by_insert[i])) {
                    same_stations = false;
                }
            }
        } else {
            same_stations = false;
        }
        
        if (same_stations) {
            test_station_tags = search_station_tags;
            if (verbose_ >= 4) cout << "Reuse search station tags for test stations ... " << endl;
        } else {
            test_station_tags.resize(test_stations.size(), 0);
        }
    }
    
    // Create a new i_search_stations with the fill value
    AnEn::SearchStationMatrix i_search_stations(
            test_stations.size(), max_num_search_stations,
            AnEn::_FILL_VALUE);

    // Check whether stations have xs and ys
    bool have_xy = test_stations.haveXY() && search_stations.haveXY();

    // The following code snippet generates i_search_stations. This is a table
    // including all the search stations for each test stations. Each row inclues
    // the index for the search stations for each test station, and the table has
    // the same number of rows as the number of test stations.
    //
    if (method_ == OneToOne) {
        // If one-to-one match is specified between the search and test stations
        //

        // Resize the table because each test station will be assigned with 
        // only one search station.
        //
        i_search_stations.resize(test_stations.size(), 1);

        // Match test and search stations
        vector<size_t> test_stations_index_in_search(0);

        handleError(find_nearest_station_match_(
                test_stations, search_stations, test_stations_index_in_search));

        // Assign the match result to the lookup table
        for (size_t i_test = 0; i_test < test_stations_index_in_search.size(); i_test++) {
            i_search_stations(i_test, 0) = test_stations_index_in_search.at(i_test);
        }

    } else if (method_ == OneToMany) {
        // If each test station is associated with many search stations that are
        // defined by nearest neighbor search or radius based search
        //

        if (!have_xy) {
            cerr << BOLDRED << "Error: Xs and ys are required for search space extension."
                    << RESET << endl;
            return (WRONG_METHOD);
        }

        if (verbose_ >= 3) cout << "Computing search space extension ... " << endl;

        if (num_nearest_stations == 0) {
            // In this case, the number of nearest neighbor stations is not 
            // defined. The SSE criteria is the distance plus the test/search
            // station tags.
            //

            if (distance == 0) {
                if (verbose_ >= 1) cerr << BOLDRED << "Error: Please specify"
                        << " distance or/and number of nearest stations to find."
                        << RESET << endl;
                return (MISSING_VALUE);
            }

#if defined(_OPENMP)
#pragma omp parallel for default(none) schedule(static) \
shared(test_stations_by_insert, search_stations, i_search_stations, \
distance, max_num_search_stations, test_station_tags, search_station_tags)
#endif
            for (size_t i_test = 0; i_test < test_stations_by_insert.size(); i_test++) {

                // Find search stations based on the distance
                vector<size_t> id_search_station =
                        search_stations.getStationsIdByDistance(
                        test_stations_by_insert[i_test].getX(),
                        test_stations_by_insert[i_test].getY(),
                        distance);

                // Assign search stations to matrix
                for (size_t i_search = 0; i_search < max_num_search_stations &&
                        i_search < id_search_station.size(); i_search++) {

                    // Convert from station ID to station index in search stations
                    size_t i_search_station = search_stations.getStationIndex(
                            id_search_station[i_search]);

                    // Check the search station tags. Only add search stations
                    // when the tag is the same with the current test station.
                    //
                    if (test_station_tags[i_test] == search_station_tags[i_search_station]) {
                        i_search_stations(i_test, i_search) = i_search_station;
                    }
                }
            }

        } else {
            // Otherwise, if the number of nearest neighbors has been assigned,
            // search space extension becomes a k-neighbor search with the extra
            // constraint on distance and test/search station tags.
            //
            search_stations.getNearestStationsIndex(
                    i_search_stations, test_stations, distance,
                    num_nearest_stations, test_station_tags, search_station_tags);
        }

        // For the one-to-group search method, two numeric vectors, one for test stations and
        // one for search stations, are required. These numeric vectors specify a tag (group) for 
        // each search/test station. If only search tags are provided, it is assumed that
        // test tags are the same.
        //
        // During the search station computation process, each test station will be
        // associated with search stations that have the same tag.
        //
        // Some caveats
        // 1. How to combine the process of distance-/nearest- based SSE and group-based SSE;
        //         Maybe the group-based SSE can be a preprocess. The distance-/nearest- based
        //         SSE can be a second filter on the SSE generated by group-based SSE. The 
        //         conventional SSE will be a group-based SSE with only 1 group for all search
        //         stations.
        //
        // 2. How to detect and reuse search station tags for test station tags;
        //         If only search station tags are provided, and search and test stations are
        //         literally the same, we can then safely reuse search station tags for test
        //         stations.
        //
        // Therefore, we probably need to refactor the method OneToMany and the subsequent
        // functions, to allow the SSE from a specific subset of search stations.
        //
        // Maybe we can also use this as a opportunity to functionalize some methods within
        // this big function.
        //

    } else {
        if (verbose_ >= 1) cerr << BOLDRED << "Error: Unknown method ("
                << method_ << ")!" << RESET << endl;
        return (UNKNOWN_METHOD);
    }

    swap(i_search_stations, i_search_stations_ref);

    if (verbose_ >= 5) {
        cout << "i_search_stations: " << i_search_stations_ref << endl;
    }

    return (SUCCESS);
}

errorType
AnEn::generateOperationalSearchTimes(
        const anenTime::Times & test_times,
        const anenTime::Times & search_times,
        const anenTime::Times & search_forecast_times,
        vector< anenTime::Times > & search_times_operational,
        vector< vector<size_t> > & i_search_times_operational,
        double max_flt) const {

    search_times_operational.clear();
    i_search_times_operational.clear();

    Functions functions(verbose_);

    anenTime::Times tmp_times;
    vector<size_t> i_tmp_times(0);

    for (const auto & test_time : test_times) {

        tmp_times.clear();

        for (const auto & search_time : search_times) {
            if (search_time + max_flt < test_time) {
                tmp_times.push_back(search_time);
            }
        }

        search_times_operational.push_back(tmp_times);

        functions.convertToIndex(tmp_times, search_forecast_times, i_tmp_times);
        i_search_times_operational.push_back(i_tmp_times);

        cout.precision(12);
        if (verbose_ >= 5) {
            cout << "Operational search times for " <<
                test_time << " are: " << tmp_times;
        } else if (verbose_ == 4) {
            cout << "[Operational] " << test_time << " has "
                << tmp_times.size() << " search times." << endl;
        }
    }

    return (SUCCESS);
}

errorType
AnEn::computeSimilarity(
        const Forecasts_array& test_forecasts,
        const Forecasts_array& search_forecasts,
        const StandardDeviation& sds,
        SimilarityMatrices& sims,
        const Observations_array& search_observations,
        const AnEn::TimeMapMatrix & mapping,
        const AnEn::SearchStationMatrix & i_search_stations,
        size_t i_observation_parameter, bool extend_observations,
        double max_par_nan, double max_flt_nan,
        anenTime::Times test_times,
        anenTime::Times search_times,
        bool operational, int max_sims_entries,
        int window_half_size) const {

    using COL_TAG_SIM = SimilarityMatrices::COL_TAG;

    // Check input
    handleError(check_input_(test_forecasts, search_forecasts, sds,
            search_observations, mapping, i_search_stations,
            i_observation_parameter, max_par_nan, max_flt_nan,
            operational, window_half_size));
    
    // Create object for functions
    Functions functions(verbose_);

    // If search/test times are not provided, assign the complete search/test
    // times from the corresponding forecasts.
    //
    vector<size_t> i_test_times(0), i_search_times(0);
    
    if (test_times.size() == 0) {
        // If test times are not provided, they are all test times available by default.
        if (verbose_ >= 5) cout << "Include all test times available" << endl;
        test_times = test_forecasts.getTimes();
        i_test_times.resize(test_forecasts.getTimesSize());
        iota(i_test_times.begin(), i_test_times.end(), 0);
    }

    handleError(functions.convertToIndex(test_times, test_forecasts.getTimes(), i_test_times));
    
    if (search_times.size() == 0) {
        if (verbose_ >= 5) cout << "Include all search times available" << endl;
        search_times = search_forecasts.getTimes();
        i_search_times.resize(search_forecasts.getTimesSize());
        iota(i_search_times.begin(), i_search_times.end(), 0);
    }
    
    size_t num_parameters = test_forecasts.getParametersSize();
    size_t num_flts = test_forecasts.getFLTsSize();
    size_t num_test_stations = test_forecasts.getStationsSize();
    size_t num_test_times = i_test_times.size();
    size_t num_search_stations = i_search_stations.size2();

    vector< vector<size_t> > i_search_times_operational(0);
    vector< anenTime::Times > search_times_operational(0);
    size_t num_search_times;
    
    // Get the max time offset of FLT
    const auto & flt_by_insert = search_forecasts.getFLTs().get<anenTime::by_insert>();
    double max_flt = flt_by_insert[num_flts - 1];
    vector<StandardDeviation> sds_operational_container;

    if (operational) {
        if (verbose_ >= 3) cout << "Operational search is selected." << endl;
        
        generateOperationalSearchTimes(test_times, search_times, search_forecasts.getTimes(),
                search_times_operational, i_search_times_operational, max_flt);

        // Operational search will recompute standard deviation during similarity
        // calculation which will generate a lot of standout messages. I want to
        // disable them.
        //
        functions.setVerbose(0);
        
        // Allocate the maximum number of search times.
        num_search_times = max_element(
                search_times_operational.begin(), search_times_operational.end(),
                [](const anenTime::Times & lhs, const anenTime::Times & rhs) {
                    return (lhs.size() < rhs.size());
                })->size();

        if (verbose_ >= 3) cout << "Compute Standard deviation for each test time ..." << endl;
        sds_operational_container.resize(num_test_times);

        // TODO:
        // To speed up the code, we might not want to re compute the standard deviation
        // for each day. We can compute for a group of 5 days or more.
        //
        for (size_t i = 0; i < num_test_times; i++) {
            if (verbose_ >= 4) cout << "Computing test time #"
                << i+1 << " (" << num_test_times << ") ..." << endl;
            functions.computeStandardDeviation(search_forecasts,
                    sds_operational_container[i], i_search_times_operational[i]);
        }
        
    } else {
        handleError(functions.convertToIndex(search_times, search_forecasts.getTimes(), i_search_times));
        num_search_times = i_search_times.size();
    }

    if (verbose_ >= 5) {
        cout << "Session information for computeSimilarity: " << endl
                << "# of parameters: " << num_parameters << endl
                << "# of FLTs: " << num_flts << endl
                << "# of test stations: " << num_test_stations << endl
                << "# of test times: " << num_test_times << endl
                << "# of maximum search stations: " << num_search_stations << endl
                << "# of observation stations: " << search_observations.getStationsSize() << endl;
        if (!operational) cout << "# of search times: " << num_search_times << endl;
    }

    // Get data
    auto & data_search_observations = search_observations.data();
    
    // Pre compute the window size for each FLT
    boost::numeric::ublas::matrix<size_t> flts_window;
    handleError(functions.computeSearchWindows(flts_window, num_flts, window_half_size));

    // Get circular parameters and parameter weights
    vector<bool> circular_flags(num_parameters, false);
    vector<double> weights(num_parameters, 1);
    auto & parameters_by_insert = test_forecasts.getParameters().get<anenPar::by_insert>();
    for (size_t i_parameter = 0; i_parameter < num_parameters; i_parameter++) {
        circular_flags[i_parameter] = parameters_by_insert[i_parameter].getCircular();
        weights[i_parameter] = parameters_by_insert[i_parameter].getWeight();
        if (std::isnan(weights[i_parameter])) weights[i_parameter] = 1;
    }

    vector<size_t> test_stations_index_in_search(0);
    if (!extend_observations) {
        // If only observations from the main (center) station are used in the analogs, I need to
        // find the corresponding search station index to the main test station.
        //
        const auto & search_observation_stations = search_observations.getStations();
        const auto & test_stations = test_forecasts.getStations();
        handleError(find_nearest_station_match_(
                test_stations, search_observation_stations, test_stations_index_in_search));
    }

    if (verbose_ >= 3) cout << "Computing similarity matrices ... " << endl;

    // Resize similarity matrices according to the search space
    if (max_sims_entries > 0) {
        sims.setMaxEntries(max_sims_entries);
    } else {
        sims.setMaxEntries(num_search_stations * num_search_times);
    }

    sims.resize(num_test_stations, num_test_times, num_flts);
    fill(sims.data(), sims.data() + sims.num_elements(), NAN);

#if defined(_OPENMP)
#pragma omp parallel for default(none) schedule(static) collapse(3) \
shared(num_test_stations, num_flts, num_search_stations, num_test_times, \
circular_flags, num_parameters, data_search_observations, functions, \
flts_window, mapping, weights, search_forecasts, test_forecasts, max_sims_entries, \
sims, sds, i_observation_parameter, i_search_stations, max_flt_nan, max_par_nan, \
test_stations_index_in_search, extend_observations, i_test_times, sds_operational_container, \
test_times, max_flt, operational, search_times_operational, i_search_times_operational) \
firstprivate(i_search_times, search_times, num_search_times)
#endif
    for (size_t i_test_station = 0; i_test_station < num_test_stations; i_test_station++) {
        for (size_t pos_test_time = 0; pos_test_time < num_test_times; pos_test_time++) {
            for (size_t i_flt = 0; i_flt < num_flts; i_flt++) {

                // The following code is serial for each thread to avoid data racing problem when max_sims_entries is set. 
                for (size_t i_search_station_index = 0; i_search_station_index < num_search_stations; i_search_station_index++) {
                    
                    // Create a partial view of the big similarity matrices.
                    // This view looks at the value column of a particular station, day, and FLT.
                    // This view will be used if max_sims_entries is set to a positive number.
                    //
                    typedef boost::multi_array_types::index_range range;
                    SimilarityMatrices::index_gen indices;
                    SimilarityMatrices::array_view<1>::type view_sim_val =
                            sims[ indices[i_test_station][pos_test_time][i_flt][range()][COL_TAG_SIM::VALUE] ];

                    size_t i_test_time = i_test_times[pos_test_time];
                    
                    if (operational) {
                        // Update search_times and the relevant
                        search_times = search_times_operational[pos_test_time];
                        i_search_times = i_search_times_operational[pos_test_time];
                    }
                    
                    double i_search_station_current = NAN,
                           i_search_station = i_search_stations(i_test_station, i_search_station_index);
                    if (!extend_observations) {
                        i_search_station_current = test_stations_index_in_search[i_test_station];

                        if (std::isnan(i_search_station_current)) {
                            // If the search station is NAN, do not continue for this search station
                            continue;
                        }
                    }

                    // Check if search station is NAN
                    if (!std::isnan(i_search_station)) {
                        for (size_t pos_search_time = 0; pos_search_time < i_search_times.size(); pos_search_time++) {
                            
                            // Determine which row in the similarity matrix should be updated
                            size_t i_search_time = i_search_times[pos_search_time];

                            // This variable determines which row in the similarity matrix will be written
                            size_t i_sim_row = i_search_station_index * num_search_times + pos_search_time;
                            
                            // We take care of the overlapping cases when part of the FLTs of search covers FLTs of test forecasts.
                            if (max_flt + search_times[pos_search_time] < test_times[pos_test_time] ||
                                    test_times[pos_test_time] + max_flt < search_times[pos_search_time]) {
                                
                                // If search and test forecasts are not overlapping

                                if (!std::isnan(mapping(i_search_time, i_flt))) {

                                    // Check whether the observation is NAN. There are two cases,
                                    // extending observations or not extending observations.
                                    //
                                    bool nan_observation = true;

                                    if (extend_observations) {
                                        // If observations are taken from the search stations
                                        nan_observation = std::isnan(data_search_observations
                                                [i_observation_parameter][i_search_station][mapping(i_search_time, i_flt)]);
                                    } else {
                                        // If observations are taken from the current test search station
                                        nan_observation = std::isnan(data_search_observations
                                                [i_observation_parameter][i_search_station_current][mapping(i_search_time, i_flt)]);
                                    }

                                    if (!nan_observation) {
                                        double sim_val = 0;

                                        // compute single similarity
                                        if (operational) {
                                            sim_val = compute_single_similarity_(
                                                    test_forecasts, search_forecasts, sds_operational_container[pos_test_time], weights, flts_window, circular_flags,
                                                    i_test_station, i_test_time, i_search_station, i_search_time, i_flt, max_par_nan, max_flt_nan);
                                        } else {
                                            sim_val = compute_single_similarity_(
                                                test_forecasts, search_forecasts, sds, weights, flts_window, circular_flags,
                                                i_test_station, i_test_time, i_search_station, i_search_time, i_flt, max_par_nan, max_flt_nan);
                                        }

                                        // Write this single value to the correct row in similarity matrix
                                        if (max_sims_entries > 0) {
                                            
                                            if (!std::isnan(sim_val)) {
                                                // Figure out which row has the biggest similarity, and check whether that row should be updated
                                                // using this calculated similarity value.
                                                //
                                                size_t i_max = distance(view_sim_val.begin(), max_element(view_sim_val.begin(), view_sim_val.end(),
                                                        [](double lhs, double rhs) {
                                                            // NAN is always treated as the largest. If both are NAN, the left one is larger.
                                                            if (std::isnan(lhs)) return false;
                                                            else if (std::isnan(rhs)) return true;
                                                            else return (lhs < rhs);
                                                        }));

                                                if (std::isnan(view_sim_val[i_max]) || view_sim_val[i_max] > sim_val) {
                                                    // Update similarity matrix
                                                    sims[i_test_station][pos_test_time][i_flt][i_max][COL_TAG_SIM::VALUE] = sim_val;
                                                    sims[i_test_station][pos_test_time][i_flt][i_max][COL_TAG_SIM::STATION] = i_search_station;
                                                    sims[i_test_station][pos_test_time][i_flt][i_max][COL_TAG_SIM::TIME] = i_search_time;
                                                }
                                            }
                                            
                                        } else {
                                            sims[i_test_station][pos_test_time][i_flt][i_sim_row][COL_TAG_SIM::VALUE] = sim_val;
                                        }


                                    } // End of isnan(observation value)
                                        
                                    // Even if the observation value is NAN, I still record the search forecast index.
                                    // The similarity metric will remain NAN which won't affect the selection.
                                    //
                                    if (max_sims_entries <= 0) {
                                        sims[i_test_station][pos_test_time][i_flt][i_sim_row][COL_TAG_SIM::STATION] = i_search_station;
                                        sims[i_test_station][pos_test_time][i_flt][i_sim_row][COL_TAG_SIM::TIME] = i_search_time;
                                    }

                                } else {
                                    if (max_sims_entries <= 0)
                                        sims[i_test_station][pos_test_time][i_flt][i_sim_row][COL_TAG_SIM::TIME] = -1;
                                } // End of isnan(mapping value)
                            } else {
                                if (max_sims_entries <= 0)
                                    sims[i_test_station][pos_test_time][i_flt][i_sim_row][COL_TAG_SIM::TIME] = -2;

                            }// End if of non-overlapping test and search forecasts

                        } // End loop of search times
                    } // End if of valid search station
                } // End loop of search stations
            } // End loop of FLTs
        } // End loop of test times
    } // End loop of test stations

    return (SUCCESS);
}

errorType
AnEn::selectAnalogs(
        Analogs & analogs,
        SimilarityMatrices & sims,
        const anenSta::Stations & test_stations,
        const Observations_array & search_observations,
        const Functions::TimeMapMatrix & mapping,
        size_t i_parameter, size_t num_members,
        bool quick, bool extend_observations) const {

    if (i_parameter >= search_observations.getParametersSize()) {
        if (verbose_ >= 1) cerr << BOLDRED << "Error: i_parameter exceeds the limits. "
                << "There are only " << search_observations.getParametersSize()
            << " parameters available." << RESET << endl;
        return (OUT_OF_RANGE);
    }
    
    size_t num_test_stations = sims.shape()[0];
    size_t num_test_times = sims.shape()[1];
    size_t num_flts = sims.shape()[2];
    size_t max_members = sims.getMaxEntries();

    if (num_members > max_members) {
        if (verbose_ >= 2) cout << RED << "Warning: Number of members ("
                << num_members << ") is bigger than the number of entries ("
                << max_members << ") in SimilarityMatrices! "
                << " NAN will exist in Analogs." << RESET << endl;
    }

    Analogs::extent_gen extents;
    analogs.resize(extents[0][0][0][0][0]);
    analogs.resize(extents[num_test_stations][num_test_times][num_flts][num_members][3]);
    fill_n(analogs.data(), analogs.num_elements(), NAN);

    using COL_TAG_ANALOG = Analogs::COL_TAG;
    using COL_TAG_SIM = SimilarityMatrices::COL_TAG;

    sims.sortRows(quick, num_members);

    const auto & data_observations = search_observations.data();

    vector<size_t> test_stations_index_in_search(0);
    if (!extend_observations) {
        // If only observations from the main (center) station are used in the analogs, I need to
        // find the corresponding search station index to the main test station.
        //
        const auto & search_observation_stations = search_observations.getStations();
        handleError(find_nearest_station_match_(
                test_stations, search_observation_stations, test_stations_index_in_search));
    }

    if (verbose_ >= 3) cout << "Selecting analogs ..." << endl;

#if defined(_OPENMP)
#pragma omp parallel for default(none) schedule(dynamic) collapse(4) \
shared(data_observations, sims, num_members, mapping, analogs, num_test_stations, \
i_parameter, num_test_times, num_flts, max_members, \
extend_observations, test_stations_index_in_search)
#endif
    for (size_t i_test_station = 0; i_test_station < num_test_stations; i_test_station++) {
        for (size_t i_test_time = 0; i_test_time < num_test_times; i_test_time++) {
            for (size_t i_flt = 0; i_flt < num_flts; i_flt++) {
                for (size_t i_member = 0; i_member < num_members; i_member++) {

                    if (i_member < max_members) {
                        if (!std::isnan(sims[i_test_station][i_test_time][i_flt][i_member][COL_TAG_SIM::VALUE])) {

                            size_t i_search_station = 0;
                            if (extend_observations) {
                                i_search_station = (size_t) sims[i_test_station][i_test_time][i_flt][i_member][COL_TAG_SIM::STATION];
                            } else {
                                i_search_station = test_stations_index_in_search[i_test_station];
                            }

                            size_t i_search_forecast_time = (size_t) sims[i_test_station][i_test_time][i_flt][i_member][COL_TAG_SIM::TIME];

                            if (!std::isnan(mapping(i_search_forecast_time, i_flt))) {
                                size_t i_observation_time = mapping(i_search_forecast_time, i_flt);

                                analogs[i_test_station][i_test_time][i_flt][i_member][COL_TAG_ANALOG::STATION] = i_search_station;
                                analogs[i_test_station][i_test_time][i_flt][i_member][COL_TAG_ANALOG::TIME] = i_observation_time;
                                analogs[i_test_station][i_test_time][i_flt][i_member][COL_TAG_ANALOG::VALUE] =
                                        data_observations[i_parameter][i_search_station][i_observation_time];
                            } else {
                                analogs[i_test_station][i_test_time][i_flt][i_member][COL_TAG_ANALOG::TIME] = -1;
                            }
                        } else {
                            analogs[i_test_station][i_test_time][i_flt][i_member][COL_TAG_ANALOG::TIME] = -2;
                        }
                    } else {
                        analogs[i_test_station][i_test_time][i_flt][i_member][COL_TAG_ANALOG::TIME] = -3;
                    }
                } // End loop of members
            } // End loop of FLTs
        } // End loop of test times
    } // End loop of test stations

    return (SUCCESS);
}

int
AnEn::getVerbose() const {
    return verbose_;
}

simMethod
AnEn::getMethod() const {
    return method_;
}

void
AnEn::setMethod(simMethod method) {
    if (method == OneToMany || method == OneToOne) this->method_ = method;
    else throw runtime_error("Failed to set an unknown method.");
}

void
AnEn::setVerbose(int verbose) {
    verbose_ = verbose;
}

errorType
AnEn::check_input_(
        const Forecasts_array& test_forecasts,
        const Forecasts_array& search_forecasts,
        const StandardDeviation& sds,
        const Observations_array& search_observations,
        const AnEn::TimeMapMatrix & mapping,
        const AnEn::SearchStationMatrix & i_search_stations,
        size_t i_observation_parameter,
        double max_par_nan, double max_flt_nan,
        bool operational, int window_half_size) const {

    size_t num_parameters = test_forecasts.getParametersSize(),
            num_flts = test_forecasts.getFLTsSize(),
            num_search_stations = search_forecasts.getStationsSize(),
            num_test_stations = test_forecasts.getStationsSize();

    // Check max_par_nan
    if (max_par_nan >= 0 && max_par_nan <= search_forecasts.getParametersSize()) {
        // valid input
    } else {
        max_par_nan = 0;
    }

    // Check max_flt_nan
    if (max_flt_nan >= 0 && max_flt_nan <= search_forecasts.getFLTsSize()) {
        // valid input
    } else {
        max_flt_nan = 0;
    }

    if (i_observation_parameter >= search_observations.getParametersSize()) {
        if (verbose_ >= 1) cerr << BOLDRED
                << "Error: Please specify a valid observation variable!"
                << RESET << endl;
        return (OUT_OF_RANGE);
    }

    if (mapping.size1() != search_forecasts.getTimesSize()) {
        if (verbose_ >= 1) cerr << BOLDRED
                << "Error: The rows of mapping should equal the number of forecast times!"
                << RESET << endl;
        return (WRONG_SHAPE);
    }

    if (mapping.size2() != search_forecasts.getFLTsSize()) {
        if (verbose_ >= 1) cerr << BOLDRED
                << "Error: The columns of mapping should equal the number of FLTs!"
                << RESET << endl;
        return (WRONG_SHAPE);
    }

    if (!operational) {
        if (!(sds.shape()[0] == search_forecasts.getParametersSize() &&
                sds.shape()[1] == search_forecasts.getStationsSize() &&
                sds.shape()[2] == search_forecasts.getFLTsSize())) {
            if (verbose_ >= 1) {
                cerr << BOLDRED << "Error: Standard deviation array has a different shape to search forecasts!" << endl;
                if (sds.shape()[0] != search_forecasts.getParametersSize())
                    cout << "-- Number of parameters differs: sds have " << sds.shape()[0]
                    << " and search forecasts have " << search_forecasts.getParametersSize() << endl;
                if (sds.shape()[1] != search_forecasts.getStationsSize())
                    cout << "-- Number of stations differs: sds have " << sds.shape()[1]
                    << " and search forecasts have " << search_forecasts.getStationsSize() << endl;
                if (sds.shape()[2] != search_forecasts.getFLTsSize())
                    cout << "-- Number of FLTs differs: sds have " << sds.shape()[2]
                    << " and search forecasts have " << search_forecasts.getFLTsSize() << endl;
                cout << RESET << endl;
            }
            return (WRONG_SHAPE);
        }   
    }

    if (num_parameters != search_forecasts.getParametersSize()) {
        if (verbose_ >= 1) cerr << BOLDRED
                << "Error: Search and test forecasts should have same numbers of parameters!"
                << RESET << endl;
        return (WRONG_SHAPE);
    }

    if (num_flts != search_forecasts.getFLTsSize()) {
        if (verbose_ >= 1) cerr << BOLDRED
                << "Error: Search and test forecasts should have same numbers of FLTs!"
                << RESET << endl;
        return (WRONG_SHAPE);
    }

    if (num_search_stations != search_observations.getStationsSize()) {
        if (verbose_ >= 1) cerr << BOLDRED
                << "Error: Search forecasts and observations should have same numbers of stations!"
                << RESET << endl;
        return (WRONG_SHAPE);
    }

    // Check i_search_stations
    size_t num_neighbor_stations = i_search_stations.size2();
    
    if (i_search_stations.size1() == 0 || num_neighbor_stations == 0) {
        if (verbose_ >= 1) cerr << BOLDRED
                << "Error: The search station lookup table is empty." << RESET << endl;
        return (MISSING_VALUE);
    }

    if (i_search_stations.size1() != num_test_stations) {
        if (verbose_ >= 1) cerr << BOLDRED
                << "Error: Number of rows in i_search_stations ("
                << i_search_stations.size1() << ") should equal the number of test stations ("
            << num_test_stations << ")." << RESET << endl;
        return (WRONG_SHAPE);
    }

    if (method_ == OneToOne) {
        if (num_neighbor_stations != 1) {
            cerr << BOLDRED << "Error: More than one search stations are assigned."
                    << RESET << endl;
            return (WRONG_METHOD);
        }
    }

    if (window_half_size < 0) {
        if (verbose_ >= 1) cerr << BOLDRED
            << "Error: Lead time comparison radius (window_half_size) cannot be negative ("
                << window_half_size << ")!" << RESET << endl;

        return (ERROR_SETTING_VALUES);
    }

    return (SUCCESS);
}

errorType
AnEn::find_nearest_station_match_(
        const anenSta::Stations& test_stations,
        const anenSta::Stations& search_stations,
        vector<size_t>& test_stations_index_in_search) const {

    if (search_stations.size() == 0) {
        if (verbose_ >= 1) cerr << BOLDRED
                << "Error: Empty station container." << RESET << endl;
        return (MISSING_VALUE);
    }

    if (test_stations.size() == 0) {
        if (verbose_ >= 1) cerr << BOLDRED
                << "Error: Empry station target." << RESET << endl;
        return (MISSING_VALUE);
    }
    
    bool same_station_order = true, same_station_numer = true;
    
    const auto & test_stations_by_insert = test_stations.get<anenSta::by_insert>();
    const auto & search_stations_by_insert = search_stations.get<anenSta::by_insert>();
    
    if (search_stations.size() == test_stations.size()) {
        for (size_t i = 0; i < search_stations.size(); i++) {
            if (!test_stations_by_insert[i].literalCompare(search_stations_by_insert[i])) {
                same_station_order = false;
                break;
            }
        }
    } else {
        same_station_numer = false;
    }
    
    if (same_station_numer && same_station_order) {
        // If location information is NOT complete, match search and test stations based on indices.
        // This assumes the numbers of test and search stations are the same.
        //
        if (verbose_ >= 3) cout << "One-on-one matching search and test stations based on indices ..." << endl;

        if (search_stations.size() != test_stations.size()) {
            cerr << BOLDRED << "Error: Search stations and test stations must be the same when no location info is provided."
                    << RESET << endl;
            return (WRONG_METHOD);
        }

        test_stations_index_in_search.resize(test_stations.size());
        iota(test_stations_index_in_search.begin(), test_stations_index_in_search.end(), 0);
        
    } else {
        // If location information is complete, find the matching search station based on locations.
        if (verbose_ >= 3) cout << "One-on-one matching search and test stations based on xs and ys ..." << endl;

        if (search_stations.haveXY() && test_stations.haveXY()) {

#if defined(_OPENMP)
            if (test_stations.size() > _SERIAL_LENGTH_LIMIT) {
                vector<size_t> index_vec(test_stations.size());

#pragma omp parallel for default(none) schedule(static) \
shared(test_stations, test_stations_by_insert, search_stations, index_vec)
                for (size_t i = 0; i < index_vec.size(); i++) {
                    const anenSta::Station & test = test_stations_by_insert[i];
                    size_t id = search_stations.getNearestStationsId(test.getX(), test.getY(), 1)[0];
                    index_vec[i] = search_stations.getStationIndex(id);
                }

                test_stations_index_in_search.insert(test_stations_index_in_search.end(), index_vec.begin(), index_vec.end());

            } else {
#endif
                for_each(test_stations_by_insert.begin(), test_stations_by_insert.end(),
                        [&test_stations_index_in_search, &search_stations](const anenSta::Station & test) {
                            size_t id = search_stations.getNearestStationsId(test.getX(), test.getY(), 1)[0];
                            test_stations_index_in_search.push_back(search_stations.getStationIndex(id));
                        });
#if defined(_OPENMP)
            }
#endif
        } else {
            if (verbose_ >= 1) cerr << BOLDRED << "Error: Cannot match stations." << RESET << endl;
            return (MISSING_VALUE);
        }

    }

    return (SUCCESS);
}

double
AnEn::compute_single_similarity_(
        const Forecasts_array & test_forecasts,
        const Forecasts_array & search_forecasts,
        const StandardDeviation & sds,
        const vector<double> & weights,
        const boost::numeric::ublas::matrix<size_t>& flts_window,
        const vector<bool> & circular_flags,

        size_t i_test_station,
        size_t i_test_time,
        size_t i_search_station,
        size_t i_search_time,
        size_t i_flt,

        double max_par_nan,
        double max_flt_nan) const {
    
    // Create function object
    Functions functions(verbose_);

    // Initialize similarity metric value.
    double sim = 0;

    // Count the number of NAN parameters.
    double num_par_nan = 0.0;

    // Define the number of parameters.
    size_t num_parameters = test_forecasts.getParametersSize();

    // Get data objects for test and search forecasts.
    const auto & data_search_forecasts = search_forecasts.data();
    const auto & data_test_forecasts = test_forecasts.data();

    for (size_t i_parameter = 0; i_parameter < num_parameters; i_parameter++) {

        if (weights[i_parameter] != 0) {
            vector<double> window(flts_window(i_flt, 1) - flts_window(i_flt, 0) + 1);
            short pos = 0;

            for (size_t i_window_flt = flts_window(i_flt, 0);
                    i_window_flt <= flts_window(i_flt, 1); i_window_flt++, pos++) {

                double value_search = data_search_forecasts
                        [i_parameter][i_search_station][i_search_time][i_window_flt];
                double value_test = data_test_forecasts
                        [i_parameter][i_test_station][i_test_time][i_window_flt];

                if (std::isnan(value_search) || std::isnan(value_test)) {
                    window[pos] = NAN;
                }

                if (circular_flags[i_parameter]) {
                    window[pos] = pow(functions.diffCircular(value_search, value_test), 2);
                } else {
                    window[pos] = pow(value_search - value_test, 2);
                }
            } // End loop of search window FLTs

            double average = functions.mean(window, max_flt_nan);

            if (!std::isnan(average)) {
                if (sds[i_parameter][i_search_station][i_flt] > 0) {
                    sim += weights[i_parameter] * (sqrt(average) / sds[i_parameter][i_search_station][i_flt]);
                }
            } else {
                // Record this nan parameter average
                num_par_nan++;

                // If a maximum of nan parameter is set
                if (num_par_nan > max_par_nan) {
                    sim = NAN;
                    break;
                }
            }

        } // End of check of the paramter weight
    } // End loop of parameters

    return (sim);
}
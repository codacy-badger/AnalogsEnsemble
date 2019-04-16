/* 
 * File:   similarityCalculator.cpp
 * Author: Weiming Hu <weiming@psu.edu>
 *
 * Created on September 11, 2018, 5:40 PM
 */

/** @file */

#if defined(_CODE_PROFILING)
#include <ctime>
#if defined(_OPENMP)
#include <omp.h>
#endif
#endif

#include "AnEnIO.h"
#include "AnEn.h"
#include "CommonExeFunctions.h"

#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"
#include "boost/exception/all.hpp"

#include <iostream>
#include <fstream>

using namespace std;

void runSimilarityCalculator(
        const string & file_test_forecasts,
        const vector<size_t> & test_start,
        const vector<size_t> & test_count,

        const vector<string> & files_search_forecasts,
        const vector<size_t> & search_start,
        const vector<size_t> & search_count,

        const vector<string> & files_observations,
        const vector<size_t> & obs_start,
        const vector<size_t> & obs_count,

        const string & file_similarity,
        const string & file_mapping,

        const string & file_sds,
        const vector<size_t> & sds_start,
        const vector<size_t> & sds_count,

        size_t observation_id,
        bool extend_observations, 
        
        bool searchExtension, size_t max_neighbors,
        size_t num_neighbors, double distance,
        int time_match_mode, double max_par_nan,
        double max_flt_nan,

        vector<size_t> test_times_index,
        vector<size_t> search_times_index,
        bool operational, int max_sims_entries, int verbose,
        const vector<double> & weights,
        int search_along, int obs_along) {

    
#if defined(_CODE_PROFILING)
    clock_t time_start = clock();
#if defined(_OPENMP)
    double wtime_start = omp_get_wtime();
#endif
#endif

    /************************************************************************
     *                         Read Input Data                              *
     ************************************************************************/
    if (verbose >= 3) cout << GREEN << "Start computing similarity ... " << RESET << endl;

    Forecasts_array test_forecasts, search_forecasts;
    Observations_array observations;

    AnEnIO io("Read", file_test_forecasts, "Forecasts", verbose),
           io_out("Write", file_similarity, "Similarity", verbose);

    if (test_start.empty() || test_count.empty()) {
        handleError(io.readForecasts(test_forecasts));
    } else {
        handleError(io.readForecasts(test_forecasts,
                test_start, test_count));
    }

    if (files_search_forecasts.size() == 1) {
        io.setFilePath(files_search_forecasts[0]);
        if (search_start.empty() || search_count.empty()) {
            handleError(io.readForecasts(search_forecasts));
        } else {
            handleError(io.readForecasts(search_forecasts,
                        search_start, search_count));
        }
    } else {
        AnEnIO::combineForecastsArray(files_search_forecasts, search_forecasts,
                search_along, verbose, search_start, search_count);
    }

    if (files_observations.size() == 1) {
        io.setFileType("Observations");
        io.setFilePath(files_observations[0]);
        if (obs_start.empty() || obs_count.empty()) {
            handleError(io.readObservations(observations));
        } else {
            handleError(io.readObservations(observations,
                        obs_start, obs_count));
        }
    } else {
        AnEnIO::combineObservationsArray(files_observations, observations,
                obs_along, verbose, obs_start, obs_count);
    }

    // Assign weights if weights are provided properly
    if (weights.size() != 0) {

        // Weights of test forecasts will be used.
        if (weights.size() == test_forecasts.getParametersSize()) {
            
            anenPar::Parameters parameters_new;
            auto & parameters_by_insert =
                test_forecasts.getParameters().get<anenPar::by_insert>();

            for (size_t i = 0; i < weights.size(); i++) {
                auto parameter = parameters_by_insert[i];
                parameter.setWeight(weights[i]);
                parameters_new.push_back(parameter);
            }

            test_forecasts.setParameters(parameters_new);

            if (verbose >= 4) cout << test_forecasts.getParameters();

        } else {
            if (verbose >= 1) {
                cerr << BOLDRED << "Error: " << weights.size() << " weights provided. But forecasts have "
                    << test_forecasts.getParametersSize() << " parameters." << RESET << endl;
                return;
            }
        }
    }

#if defined(_CODE_PROFILING)
    clock_t time_end_of_reading = clock();
#if defined(_OPENMP)
    double wtime_end_of_reading = omp_get_wtime();
#endif
#endif


    /************************************************************************
     *                     Similarity Computation                           *
     ************************************************************************/
    AnEn anen(verbose);
    Functions functions(verbose);

    SimilarityMatrices sims(test_forecasts);

    StandardDeviation sds;
    if (!operational) {
        if (!file_sds.empty()) {
            // If the standard deviation file is provided, read the file
            AnEnIO io_sds("Read", file_sds, "StandardDeviation", verbose);

            if (sds_start.empty() || sds_count.empty()) {
                handleError(io_sds.readStandardDeviation(sds));
            } else {
                handleError(io_sds.readStandardDeviation(sds, sds_start, sds_count));
            }
        } else {
            handleError(functions.computeStandardDeviation(
                        search_forecasts, sds, search_times_index));
        }
    }

#if defined(_CODE_PROFILING)
    clock_t time_end_of_sd = clock();
#if defined(_OPENMP)
    double wtime_end_of_sd = omp_get_wtime();
#endif
#endif

    AnEn::TimeMapMatrix mapping;

    bool file_mapping_exists = false;
    if (!file_mapping.empty()) file_mapping_exists = boost::filesystem::exists(file_mapping);

    if (file_mapping_exists) {
        AnEnIO io_mapping("Read", file_mapping, "Matrix", verbose);
        io_mapping.readTextMatrix(mapping);
    } else {
        // Compute mapping from forecast time/flts to observation times
        handleError(functions.computeObservationsTimeIndices(
                    search_forecasts.getTimes(), search_forecasts.getFLTs(),
                    observations.getTimes(), mapping, time_match_mode));

        if (!file_mapping.empty()) {
            io.setMode("Write", file_mapping);
            io.setFileType("Matrix");
            handleError(io.writeTextMatrix(mapping));
        }
    }

#if defined(_CODE_PROFILING)
    clock_t time_end_of_mapping = clock();
#if defined(_OPENMP)
    double wtime_end_of_mapping = omp_get_wtime();
#endif
#endif

    if (searchExtension) {
        anen.setMethod(AnEn::simMethod::OneToMany);
    } else {
        anen.setMethod(AnEn::simMethod::OneToOne);
    }
    
    AnEn::SearchStationMatrix i_search_stations;

    handleError(anen.computeSearchStations(
            test_forecasts.getStations(),
            search_forecasts.getStations(),
            i_search_stations, max_neighbors,
            distance, num_neighbors));
    
    anenTime::Times test_times, search_times;

    const auto & container_test = test_forecasts.getTimes().get<anenTime::by_insert>();
    for (size_t i : test_times_index) test_times.push_back(container_test[i]);

    const auto & container_search = search_forecasts.getTimes().get<anenTime::by_insert>();
    for (size_t i : search_times_index) search_times.push_back(container_search[i]);

    handleError(anen.computeSimilarity(
            test_forecasts, search_forecasts, sds, sims, observations, mapping,
            i_search_stations, observation_id, extend_observations,
            max_par_nan, max_flt_nan, test_times, search_times, operational, max_sims_entries));

#if defined(_CODE_PROFILING)
    clock_t time_end_of_sim = clock();
#if defined(_OPENMP)
    double wtime_end_of_sim = omp_get_wtime();
#endif
#endif


    /************************************************************************
     *                         Write Similarity                             *
     ************************************************************************/
    handleError(io_out.writeSimilarityMatrices(
            sims, test_forecasts.getParameters(),
            test_forecasts.getStations(),
            test_forecasts.getTimes(),
            test_forecasts.getFLTs(),
            search_forecasts.getStations(),
            search_forecasts.getTimes()));

    if (verbose >= 3) cout << GREEN << "Done!" << RESET << endl;

#if defined(_CODE_PROFILING)
    clock_t time_end_of_write = clock();
#if defined(_OPENMP)
    double wtime_end_of_write = omp_get_wtime();

    double wduration_full = wtime_end_of_write - wtime_start,
           wduration_reading = wtime_end_of_reading - wtime_start,
           wduration_sd = wtime_end_of_sd - wtime_end_of_reading,
           wduration_mapping = wtime_end_of_mapping - wtime_end_of_sd,
           wduration_sim = wtime_end_of_sim - wtime_end_of_mapping,
           wduration_write = wtime_end_of_write - wtime_end_of_sim,
           wduration_computation = wduration_sd + wduration_mapping + wduration_sim;
#endif

    float duration_full = (float) (time_end_of_write - time_start) / CLOCKS_PER_SEC,
          duration_reading = (float) (time_end_of_reading - time_start) / CLOCKS_PER_SEC,
          duration_sd = (float) (time_end_of_sd - time_end_of_reading) / CLOCKS_PER_SEC,
          duration_mapping = (float) (time_end_of_mapping - time_end_of_sd) / CLOCKS_PER_SEC,
          duration_sim = (float) (time_end_of_sim - time_end_of_mapping) / CLOCKS_PER_SEC,
          duration_write = (float) (time_end_of_write - time_end_of_sim) / CLOCKS_PER_SEC;
    float duration_computation = duration_sd + duration_mapping + duration_sim;
    cout << "-----------------------------------------------------" << endl
        << "User (CPU) Time for Similarity Calculator:" << endl
        << "Total: " << duration_full << " seconds (100%)" << endl
        << "Reading data: " << duration_reading << " seconds (" << duration_reading / duration_full * 100 << "%)" << endl
        << "Computation: " << duration_computation << " seconds (" << duration_computation / duration_full * 100 << "%)" << endl
        << " -- SD: " << duration_sd << " seconds (" << duration_sd / duration_full * 100 << "%)" << endl
        << " -- Mapping: " << duration_mapping << " seconds (" << duration_mapping / duration_full * 100 << "%)" << endl
        << " -- Similarity: " << duration_sim << " seconds (" << duration_sim / duration_full * 100 << "%)" << endl
        << "Writing data: " << duration_write << " seconds (" << duration_write / duration_full * 100 << "%)" << endl
        << "-----------------------------------------------------" << endl;
#if defined(_OPENMP)
    cout << "-----------------------------------------------------" << endl
        << "Real (Wall) Time for Similarity Calculator:" << endl
        << "Total: " << wduration_full << " seconds (100%)" << endl
        << "Reading data: " << wduration_reading << " seconds (" << wduration_reading / wduration_full * 100 << "%)" << endl
        << "Computation: " << wduration_computation << " seconds (" << wduration_computation / wduration_full * 100 << "%)" << endl
        << " -- SD: " << wduration_sd << " seconds (" << wduration_sd / wduration_full * 100 << "%)" << endl
        << " -- Mapping: " << wduration_mapping << " seconds (" << wduration_mapping / wduration_full * 100 << "%)" << endl
        << " -- Similarity: " << wduration_sim << " seconds (" << wduration_sim / wduration_full * 100 << "%)" << endl
        << "Writing data: " << wduration_write << " seconds (" << wduration_write / wduration_full * 100 << "%)" << endl
        << "-----------------------------------------------------" << endl;
#endif
#endif

    return;
}

int main(int argc, char** argv) {

#if defined(_ENABLE_MPI)
    AnEnIO::handle_MPI_Init();
#endif

    namespace po = boost::program_options;
    
    // Required variables
    string file_test_forecasts, file_similarity;
    vector<string> files_observations, files_search_forecasts;

    // Optional variables
    vector<double> weights;
    vector<string> config_files;
    int verbose = 0, time_match_mode = 1, search_along = 0,
        obs_along = 0, max_sims_entries = -1;
    size_t observation_id = 0, max_neighbors = 0, num_neighbors = 0;
    string file_mapping, file_sds;
    bool searchExtension = false, extend_observations = false,
            operational = false, continuous_time_index = false;
    double distance = 0.0, max_par_nan = 0.0, max_flt_nan = 0.0;
    vector<size_t> test_start, test_count, search_start, search_count,
            obs_start, obs_count, sds_start, sds_count, test_times_index,
            search_times_index;
    
    try {
        po::options_description desc("Available options");
        desc.add_options()
                ("help,h", "Print help information for options.")
                ("config,c", po::value< vector<string> >(&config_files), "Set the configuration file(s). Command line options overwrite options in configuration file. ")

                ("test-forecast-nc", po::value<string>(&file_test_forecasts)->required(), "Set the input file path for test forecast NetCDF.")
                ("search-forecast-nc", po::value< vector<string> >(&files_search_forecasts)->multitoken()->required(), "Set input the file path(s) for search forecast NetCDF.")
                ("observation-nc", po::value<vector<string> >(&files_observations)->multitoken()->required(), "Set the input file path(s) for search observation NetCDF.")
                ("similarity-nc", po::value<string>(&file_similarity)->required(), "Set the output file path for similarity NetCDF.")

                ("verbose,v", po::value<int>(&verbose)->default_value(2), "Set the verbose level.")
                ("time-match-mode", po::value<int>(&time_match_mode)->default_value(1), "Set the match mode for generating TimeMapMatrix. 0 for strict and 1 for loose search.")
                ("max-par-nan", po::value<double>(&max_par_nan)->default_value(0), "The number of NAN values allowed when computing similarity across different parameters. Set it to a negative number (will be automatically converted to NAN) to allow any number of NAN values.")
                ("max-flt-nan", po::value<double>(&max_flt_nan)->default_value(0), "The number of NAN values allowed when computing FLT window averages. Set it to a negative number (will be automatically converted to NAN) to allow any number of NAN values.")
                ("sds-nc", po::value<string>(&file_sds), "Set the file path to read for standard deviation.")
                ("mapping-txt", po::value<string>(&file_mapping), "Set the file path for mapping. If the file already exists, the file will be used; if the file does not exist, it will be created..")
                ("observation-id", po::value<size_t>(&observation_id)->default_value(0), "Set the index of the observation variable that will be used.")
                ("searchExtension", po::bool_switch(&searchExtension)->default_value(false), "Use search extension.")
                ("distance", po::value<double>(&distance)->default_value(0.0), "Set the radius for selecting neighbors.")
                ("extend-obs", po::bool_switch(&extend_observations)->default_value(false), "After getting the most similar forecast indices, take the corresponding observations from the search station.")
                ("max-neighbors", po::value<size_t>(&max_neighbors)->default_value(0), "Set the maximum neighbors allowed to keep.")
                ("num-neighbors", po::value<size_t>(&num_neighbors)->default_value(0), "Set the number of neighbors to find.")
                ("test-start", po::value< vector<size_t> >(&test_start)->multitoken(), "(File I/O) Set the start indices in the test forecast NetCDF where the program starts reading.")
                ("test-count", po::value< vector<size_t> >(&test_count)->multitoken(), "(File I/O) Set the count numbers for each dimension in the test forecast NetCDF.")
                ("search-start", po::value< vector<size_t> >(&search_start)->multitoken(), "(File I/O) Set the start indices in the search forecast NetCDF where the program starts reading.")
                ("search-count", po::value< vector<size_t> >(&search_count)->multitoken(), "(File I/O) Set the count numbers for each dimension in the search forecast NetCDF.")
                ("obs-start", po::value< vector<size_t> >(&obs_start)->multitoken(), "(File I/O) Set the start indices in the search observation NetCDF where the program starts reading.")
                ("obs-count", po::value< vector<size_t> >(&obs_count)->multitoken(), "(File I/O) Set the count numbers for each dimension in the search observation NetCDF.")
                ("sds-start", po::value< vector<size_t> >(&sds_start)->multitoken(), "(File I/O) Set the start indices in the standard deviation NetCDF where the program starts reading.")
                ("sds-count", po::value< vector<size_t> >(&sds_count)->multitoken(), "(File I/O) Set the count numbers for each dimension in the standard deviation NetCDF.")
                ("test-times-index", po::value< vector<size_t> >(&test_times_index)->multitoken(), "Set the indices or the index range (with --continuous-time) of test times in the actual forecasts after the reading.")
                ("search-times-index", po::value< vector<size_t> >(&search_times_index)->multitoken(), "Set the indices or the index range (with --continuous-time) of search times in the actual forecasts after the reading.")
                ("operational", po::bool_switch(&operational)->default_value(false), "Use operational search. This feature uses all the times from search times that are historical to each test time during comparison.")
                ("max-num-sims", po::value<int>(&max_sims_entries)->default_value(-1), "The maximum number of similarities to keep. This can be set to a positive value to reduce the memory requirement sacrificing execution time.")
                ("continuous-time", po::bool_switch(&continuous_time_index)->default_value(false), "Whether to generate a sequence for test-times-index and search-times-index. If used, an inclusive sequence is generated when only 2 indices (start and end) are provided in test or search times index.")
                ("weights", po::value<vector<double> >(&weights)->multitoken(), "Specify the weight for each parameter in forecasts. The order of weights should be the same as the order of forecast parameters.")
                ("search-along", po::value<int>(&search_along)->default_value(0), "If multiple files are provided for search forecasts, this specifies the dimension to be appended. [0:parameters, 1:stations, 2:times, 3:FLTs]. Otherwise, it is ignored.")
                ("obs-along", po::value<int>(&obs_along)->default_value(0), "If multiple files are provided for observations, this specifies the dimension to be appended. [0:parameters 1:stations 2:times]. Otherwise, it is ignored.");
        
        // process unregistered keys and notify users about my guesses
        vector<string> available_options;
        auto lambda = [&available_options](const boost::shared_ptr<boost::program_options::option_description> option) {
            available_options.push_back("--" + option->long_name());
        };
        for_each(desc.options().begin(), desc.options().end(), lambda);

        // Parse the command line options first
        po::variables_map vm;
        po::parsed_options parsed = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
        store(parsed, vm);


        cout << BOLDGREEN << "Parallel Ensemble Forecasts --- Similarity Calculator "
#if defined(_CODE_PROFILING)
            << "(with code profiling) "
#endif
            << _APPVERSION << RESET << endl << GREEN << _COPYRIGHT_MSG << RESET << endl;


        if (vm.count("help") || argc == 1) {
            cout << desc << endl;
            return 0;
        }
        
        auto unregistered_keys = po::collect_unrecognized(parsed.options, po::exclude_positional);
        if (unregistered_keys.size() != 0) {
            guess_arguments(unregistered_keys, available_options);
            return 1;
        }
        
        // Then parse the configuration file
        if (vm.count("config")) {
            // If configuration file is specified, read it first.
            // The variable won't be written until we call notify.
            //
            config_files = vm["config"].as< vector<string> >();
        }
        
        if (!config_files.empty()) {
            for (const auto & config_file : config_files) {
                ifstream ifs(config_file.c_str());
                if (!ifs) {
                    cerr << BOLDRED << "Error: Can't open configuration file " << config_file << RESET << endl;
                    return 1;
                } else {
                    auto parsed_config = parse_config_file(ifs, desc, true);

                    auto unregistered_keys_config = po::collect_unrecognized(parsed_config.options, po::exclude_positional);
                    if (unregistered_keys_config.size() != 0) {
                        guess_arguments(unregistered_keys_config, available_options);
                        return 1;
                    }

                    store(parsed_config, vm);
                }
            }
        }

        notify(vm);

        if (vm.count("max-par-nan")) {
            if (vm["max-par-nan"].as<double>() < 0) {
                max_par_nan = NAN;
            }
        }

        if (vm.count("max-flt-nan")) {
            if (vm["max-flt-nan"].as<double>() < 0) {
                max_flt_nan = NAN;
            }
        }
        
    } catch (...) {
        handle_exception(current_exception());
        return 1;
    }

    if (continuous_time_index) {
        bool succeed = false;

        if (test_times_index.size() == 2) {
            test_times_index.resize(test_times_index[1] - test_times_index[0] + 1);
            iota(test_times_index.begin(), test_times_index.end(), test_times_index[0]);
            succeed = true;

            if (verbose >= 4) {
                cout << "A sequence of test times are generated: " << test_times_index << endl;
            }
        }

        if (search_times_index.size() == 2) {
            search_times_index.resize(search_times_index[1] - search_times_index[0] + 1);
            iota(search_times_index.begin(), search_times_index.end(), search_times_index[0]);
            succeed = true;

            if (verbose >= 4) {
                cout << "A sequence of search times are generated: " << search_times_index << endl;
            }
        }

        if (!succeed) {
            cerr << BOLDRED << "Error: You have used --continuous-time, but "
                    << "both --test-times-index and --search-times-index cannot "
                    << "be converted to a range." << RESET << endl;
            return 1;
        }
    }
    
    if (verbose >= 5) {
        cout << "Input parameters:" << endl
                << "file_test_forecasts: " << file_test_forecasts << endl
                << "files_search_forecasts: " << files_search_forecasts << endl
                << "files_observations: " << files_observations << endl
                << "file_similarity: " << file_similarity << endl
                << "file_mapping: " << file_mapping << endl
                << "file_sds: " << file_sds << endl
                << "config_files: " << config_files << endl
                << "verbose: " << verbose << endl
                << "max_par_nan: " << max_par_nan << endl
                << "max_flt_nan: " << max_flt_nan << endl
                << "extend_observations: " << extend_observations << endl
                << "time_match_mode: " << time_match_mode << endl
                << "observation_id: " << observation_id << endl
                << "searchExtension: " << searchExtension << endl
                << "distance: " << distance << endl
                << "max_neighbors: " << max_neighbors << endl
                << "num_neighbors: " << num_neighbors << endl
                << "test_start: " << test_start << endl
                << "test_count: " << test_count << endl
                << "search_start: " << search_start << endl
                << "search_count: " << search_count << endl
                << "obs_start: " << obs_start << endl
                << "obs_count: " << obs_count << endl
                << "sds_start: " << sds_start << endl
                << "sds_count: " << sds_count << endl
                << "test_times_index: " << test_times_index << endl
                << "search_times_index: " << search_times_index << endl
                << "operational: " << operational << endl
                << "max_sims_entries: " << max_sims_entries << endl
                << "weights: " << weights << endl
                << "search_along: " << search_along << endl
                << "obs_along: " << obs_along << endl;
    }
    
    try {
        runSimilarityCalculator(
                file_test_forecasts, test_start, test_count,
                files_search_forecasts, search_start, search_count,
                files_observations, obs_start, obs_count, file_similarity,
                file_mapping, file_sds, sds_start, sds_count,
                observation_id, extend_observations, searchExtension,
                max_neighbors, num_neighbors, distance, time_match_mode,
                max_par_nan, max_flt_nan, test_times_index, search_times_index,
                operational, max_sims_entries, verbose, weights, search_along, obs_along);
    } catch (...) {
        handle_exception(current_exception());
        return 1;
    }
    
#if defined(_ENABLE_MPI)
    AnEnIO::handle_MPI_Finalize();
#endif

    return 0;
}


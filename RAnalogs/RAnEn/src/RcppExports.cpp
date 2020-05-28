// Generated by using Rcpp::compileAttributes() -> do not edit by hand
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include <Rcpp.h>

using namespace Rcpp;

// checkOpenMP
bool checkOpenMP();
RcppExport SEXP _RAnEn_checkOpenMP() {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    rcpp_result_gen = Rcpp::wrap(checkOpenMP());
    return rcpp_result_gen;
END_RCPP
}
// setNumThreads
void setNumThreads(int threads);
RcppExport SEXP _RAnEn_setNumThreads(SEXP threadsSEXP) {
BEGIN_RCPP
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< int >::type threads(threadsSEXP);
    setNumThreads(threads);
    return R_NilValue;
END_RCPP
}
// getNumThreads
int getNumThreads();
RcppExport SEXP _RAnEn_getNumThreads() {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    rcpp_result_gen = Rcpp::wrap(getNumThreads());
    return rcpp_result_gen;
END_RCPP
}
// generateAnalogs
List generateAnalogs(SEXP sx_forecasts, SEXP sx_observations, SEXP sx_test_times, SEXP sx_search_times, SEXP sx_config, SEXP sx_algorithm);
RcppExport SEXP _RAnEn_generateAnalogs(SEXP sx_forecastsSEXP, SEXP sx_observationsSEXP, SEXP sx_test_timesSEXP, SEXP sx_search_timesSEXP, SEXP sx_configSEXP, SEXP sx_algorithmSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type sx_forecasts(sx_forecastsSEXP);
    Rcpp::traits::input_parameter< SEXP >::type sx_observations(sx_observationsSEXP);
    Rcpp::traits::input_parameter< SEXP >::type sx_test_times(sx_test_timesSEXP);
    Rcpp::traits::input_parameter< SEXP >::type sx_search_times(sx_search_timesSEXP);
    Rcpp::traits::input_parameter< SEXP >::type sx_config(sx_configSEXP);
    Rcpp::traits::input_parameter< SEXP >::type sx_algorithm(sx_algorithmSEXP);
    rcpp_result_gen = Rcpp::wrap(generateAnalogs(sx_forecasts, sx_observations, sx_test_times, sx_search_times, sx_config, sx_algorithm));
    return rcpp_result_gen;
END_RCPP
}
// generateSearchStations
NumericMatrix generateSearchStations(SEXP sx_xs, SEXP sx_ys, int num_neighbors, SEXP sx_distance);
RcppExport SEXP _RAnEn_generateSearchStations(SEXP sx_xsSEXP, SEXP sx_ysSEXP, SEXP num_neighborsSEXP, SEXP sx_distanceSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type sx_xs(sx_xsSEXP);
    Rcpp::traits::input_parameter< SEXP >::type sx_ys(sx_ysSEXP);
    Rcpp::traits::input_parameter< int >::type num_neighbors(num_neighborsSEXP);
    Rcpp::traits::input_parameter< SEXP >::type sx_distance(sx_distanceSEXP);
    rcpp_result_gen = Rcpp::wrap(generateSearchStations(sx_xs, sx_ys, num_neighbors, sx_distance));
    return rcpp_result_gen;
END_RCPP
}
// generateTimeMapping
NumericMatrix generateTimeMapping(SEXP sx_fcst_times, SEXP sx_fcst_flts, SEXP sx_obs_times);
RcppExport SEXP _RAnEn_generateTimeMapping(SEXP sx_fcst_timesSEXP, SEXP sx_fcst_fltsSEXP, SEXP sx_obs_timesSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type sx_fcst_times(sx_fcst_timesSEXP);
    Rcpp::traits::input_parameter< SEXP >::type sx_fcst_flts(sx_fcst_fltsSEXP);
    Rcpp::traits::input_parameter< SEXP >::type sx_obs_times(sx_obs_timesSEXP);
    rcpp_result_gen = Rcpp::wrap(generateTimeMapping(sx_fcst_times, sx_fcst_flts, sx_obs_times));
    return rcpp_result_gen;
END_RCPP
}

RcppExport SEXP _rcpp_module_boot_Config();

static const R_CallMethodDef CallEntries[] = {
    {"_RAnEn_checkOpenMP", (DL_FUNC) &_RAnEn_checkOpenMP, 0},
    {"_RAnEn_setNumThreads", (DL_FUNC) &_RAnEn_setNumThreads, 1},
    {"_RAnEn_getNumThreads", (DL_FUNC) &_RAnEn_getNumThreads, 0},
    {"_RAnEn_generateAnalogs", (DL_FUNC) &_RAnEn_generateAnalogs, 6},
    {"_RAnEn_generateSearchStations", (DL_FUNC) &_RAnEn_generateSearchStations, 4},
    {"_RAnEn_generateTimeMapping", (DL_FUNC) &_RAnEn_generateTimeMapping, 3},
    {"_rcpp_module_boot_Config", (DL_FUNC) &_rcpp_module_boot_Config, 0},
    {NULL, NULL, 0}
};

RcppExport void R_init_RAnEn(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}

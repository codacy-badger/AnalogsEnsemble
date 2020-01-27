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
// validateConfiguration
void validateConfiguration(const SEXP& sx_config);
RcppExport SEXP _RAnEn_validateConfiguration(SEXP sx_configSEXP) {
BEGIN_RCPP
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const SEXP& >::type sx_config(sx_configSEXP);
    validateConfiguration(sx_config);
    return R_NilValue;
END_RCPP
}
// computeAnEnIS
SEXP computeAnEnIS(SEXP sx_config);
RcppExport SEXP _RAnEn_computeAnEnIS(SEXP sx_configSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type sx_config(sx_configSEXP);
    rcpp_result_gen = Rcpp::wrap(computeAnEnIS(sx_config));
    return rcpp_result_gen;
END_RCPP
}
// configAnEnIS
SEXP configAnEnIS();
RcppExport SEXP _RAnEn_configAnEnIS() {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    rcpp_result_gen = Rcpp::wrap(configAnEnIS());
    return rcpp_result_gen;
END_RCPP
}
// getConfigNames
SEXP getConfigNames();
RcppExport SEXP _RAnEn_getConfigNames() {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    rcpp_result_gen = Rcpp::wrap(getConfigNames());
    return rcpp_result_gen;
END_RCPP
}

static const R_CallMethodDef CallEntries[] = {
    {"_RAnEn_checkOpenMP", (DL_FUNC) &_RAnEn_checkOpenMP, 0},
    {"_RAnEn_validateConfiguration", (DL_FUNC) &_RAnEn_validateConfiguration, 1},
    {"_RAnEn_computeAnEnIS", (DL_FUNC) &_RAnEn_computeAnEnIS, 1},
    {"_RAnEn_configAnEnIS", (DL_FUNC) &_RAnEn_configAnEnIS, 0},
    {"_RAnEn_getConfigNames", (DL_FUNC) &_RAnEn_getConfigNames, 0},
    {NULL, NULL, 0}
};

RcppExport void R_init_RAnEn(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}

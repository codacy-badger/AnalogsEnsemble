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

RcppExport SEXP _rcpp_module_boot_Config();

static const R_CallMethodDef CallEntries[] = {
    {"_RAnEn_checkOpenMP", (DL_FUNC) &_RAnEn_checkOpenMP, 0},
    {"_rcpp_module_boot_Config", (DL_FUNC) &_rcpp_module_boot_Config, 0},
    {NULL, NULL, 0}
};

RcppExport void R_init_RAnEn(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}

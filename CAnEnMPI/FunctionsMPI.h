/* 
 * File:   FunctionsMPI.h
 * Author: Weiming Hu <weiming@psu.edu>
 *
 * Created on March, 2020, 11:10 AM
 */


#ifndef FUNCTIONSMPI_H
#define FUNCTIONSMPI_H

#include <vector>

#include "AnEnIS.h"
#include "Forecasts.h"
#include "Observations.h"

namespace FunctionsMPI {

    void scatterObservations(const Observations & send, Observations & recv, int num_procs, int rank, Verbose verbose);
    void scatterForecasts(const Forecasts & send, Forecasts & recv, int num_procs, int rank, Verbose verbose);
    void scatterBasicData(const BasicData & send, BasicData & recv, int num_procs, int rank, Verbose verbose);
    void scatterArray(const Array4D & send, Array4D & recv, int num_procs, int rank, Verbose verbose);

    void broadcastVector(const std::vector<std::size_t> & send, std::vector<std::size_t> & recv, int rank, Verbose verbose);
    void broadcastVector(const std::vector<bool> & send, std::vector<bool> & recv, int rank, Verbose verbose);

    void gatherArray(Array4D & arr, int station_dim_index, int num_procs, int rank, Verbose verbose);
};

#endif /* FUNCTIONSMPI_H */

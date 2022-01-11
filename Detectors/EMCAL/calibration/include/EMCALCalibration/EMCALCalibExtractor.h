// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \class EMCALCalibExtractor
/// \brief  Use the EMCal Calibration
/// \author Hannah Bossi, Yale University
/// \ingroup EMCALCalib
/// \since Oct 3, 2021

#ifndef EMCALCALIBEXTRACTOR_H_
#define EMCALCALIBEXTRACTOR_H_

#include <iostream>
#include "CCDB/BasicCCDBManager.h"
#include "EMCALCalib/BadChannelMap.h"
#include "EMCALCalib/TimeCalibrationParams.h"
#include "CommonUtils/BoostHistogramUtils.h"
#include "EMCALBase/Geometry.h"
#include <boost/histogram.hpp>

#if (defined(WITH_OPENMP) && !defined(__CLING__))
#include <omp.h>
#endif

namespace o2
{
namespace emcal
{

class EMCALCalibExtractor
{
  using boostHisto = boost::histogram::histogram<std::tuple<boost::histogram::axis::regular<double, boost::use_default, boost::use_default, boost::use_default>, boost::histogram::axis::integer<>>, boost::histogram::unlimited_storage<std::allocator<char>>>;

 public:
  EMCALCalibExtractor() = default;
  ~EMCALCalibExtractor() = default;

  o2::emcal::Geometry* mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
  int NCELLS = mGeometry->GetNCells();

  int getNsigma() const { return mSigma; }
  void setNsigma(int ns) { mSigma = ns; }

  void setNThreads(int n) { mNThreads = std::min(n, NCELLS); }
  int getNThreads() const { return mNThreads; }

  void setUseScaledHistoForBadChannels(bool useScaledHistoForBadChannels) { mUseScaledHistoForBadChannels = useScaledHistoForBadChannels; }

  /// \brief Average energy per hit is caluclated for each cell.
  /// \param emin -- min. energy for cell amplitudes
  /// \param emax -- max. energy for cell amplitudes
  boostHisto buildHitAndEnergyMean(double emin, double emax, boostHisto mCellAmplitude);
  /// \brief Scaled hits per cell
  /// \param emin -- min. energy for cell amplitudes
  /// \param emax -- max. energy for cell amplitudes
  boostHisto buildHitAndEnergyMeanScaled(double emin, double emax, boostHisto mCellAmplitude);

  /// \brief Function to perform the calibration of bad channels
  template <typename... axes>
  o2::emcal::BadChannelMap calibrateBadChannels(boost::histogram::histogram<axes...>& hist)
  {
    // calculate the mean and sigma of the mEsumHisto
    auto fitValues = o2::utils::fitBoostHistoWithGaus<double>(hist);
    double mean = fitValues.at(1);
    double sigma = fitValues.at(2);

    // calculate the "good cell window from the mean"
    double maxVal = mean + mSigma * sigma;
    double minVal = mean - mSigma * sigma;
    o2::emcal::BadChannelMap mOutputBCM;
    // now loop through the cells and determine the mask for a given cell
    for (int cellID = 0; cellID < 17664; cellID++) {
      double E = hist.at(cellID);
      // if in the good window, mark the cell as good
      // for now we won't do warm cells - the definition of this is unclear.
      if (E == 0) {
        mOutputBCM.addBadChannel(cellID, o2::emcal::BadChannelMap::MaskType_t::DEAD_CELL);
      } else if (E < maxVal || E > minVal) {
        mOutputBCM.addBadChannel(cellID, o2::emcal::BadChannelMap::MaskType_t::GOOD_CELL);
      } else {
        mOutputBCM.addBadChannel(cellID, o2::emcal::BadChannelMap::MaskType_t::BAD_CELL);
      }
    }

    return mOutputBCM;
  }

  /// \brief For now a dummy function to calibrate the time.
  template <typename... axes>
  o2::emcal::TimeCalibrationParams calibrateTime(boost::histogram::histogram<axes...>& hist, double minTime = 0, double maxTime = 1000)
  {

    auto histReduced = boost::histogram::algorithm::reduce(hist, boost::histogram::algorithm::shrink(minTime, maxTime), boost::histogram::algorithm::shrink(0, NCELLS));

    o2::emcal::TimeCalibrationParams TCP;

#if (defined(WITH_OPENMP) && !defined(__CLING__))
    if (mNThreads < 1) {
      mNThreads = std::min(omp_get_max_threads(), NCELLS);
    }
    LOG(info) << "Number of threads that will be used = " << mNThreads;
#pragma omp parallel for num_threads(mNThreads)
#else
    LOG(info) << "OPEN MP will not be used for the time calibration";
    mNThreads = 1;
#endif

    for (unsigned int i = 0; i < NCELLS; ++i) {
      // project boost histogram to 1d just for 1 cell
      auto boostHist1d = o2::utils::ProjectBoostHistoX(histReduced, i, i + 1);
      // maybe cut histo to max +- 25 ns

      // fit with gaussian to extract mean
      try {
        auto fitValues = o2::utils::fitBoostHistoWithGaus<double>(boostHist1d);
        double mean = fitValues.at(1);
        // add mean to time calib params
        TCP.addTimeCalibParam(i, mean, 0);
      } catch (o2::utils::FitGausError_t) {
        TCP.addTimeCalibParam(i, 400, 0); // 400 ns shift default value
      }
    }
    return TCP;
  }

 private:
  bool mUseScaledHistoForBadChannels = false; ///< variable to specify whether or not we want to use the scaled histo for the claibration of bad channels.
  int mSigma = 4;                             ///< number of sigma used in the calibration to define outliers
  int mNThreads = 1;                          ///< number of threads used for calibration

  ClassDefNV(EMCALCalibExtractor, 1);
};
} // namespace emcal
} // namespace o2
#endif
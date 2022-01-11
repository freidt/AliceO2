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

/// \file runTimeCalibOffline.cxx
/// \author

#include <iostream>
#include <boost/program_options.hpp>

#include "FairLogger.h"

#include "CommonUtils/BoostHistogramUtils.h"
#include "EMCALCalib/TimeCalibrationParams.h"
#include "EMCALCalib/CalibDB.h"
#include "EMCALCalibration/EMCALCalibExtractor.h"

#include "CCDB/CcdbApi.h"

#include "TFile.h"
#include "TH1.h"
#include "TH2.h"

using CcdbApi = o2::ccdb::CcdbApi;
namespace bpo = boost::program_options;

int main(int argc, char** argv)
{

  bpo::variables_map vm;
  bpo::options_description opt_general("");
  bpo::options_description opt_hidden("");
  bpo::options_description opt_all;
  bpo::positional_options_description opt_pos;

  std::string CalibInputPath;
  std::string ccdbServerPath;
  bool doBadChannelCalib;
  std::string nameCalibInputHist; // hCellIdVsTimeAbove300 for time, hCellIdVsEnergy for bad channel

  unsigned int nthreads; // number of threads used by openMP

  unsigned long rangestart; //30/10/2021, 01:02:32 for run 505566 -> 1635548552000
  unsigned long rangeend;   // 30/10/2021, 02:31:10 for run 505566 -> 1635553870000

  try {
    bpo::options_description desc("Allowed options");
    desc.add_options()("help", "Print this help message")("CalibInputPath", bpo::value<std::string>()->required(), "Set root input histogram")("ccdbServerPath", bpo::value<std::string>()->default_value(o2::base::NameConf::getCCDBServer()), "Set path to ccdb server")("mode", bpo::value<std::string>()->required(), "Set if time or bad channel calib")("nameInputHisto", bpo::value<std::string>()->default_value("hCellIdVsTimeAbove300"), "Set name of input histogram")("nthreads", bpo::value<unsigned int>()->default_value(1), "Set number of threads for OpenMP")("timestampStart", bpo::value<unsigned long>()->default_value(1635548552000), "Set timestamp from start of run")("timestampEnd", bpo::value<unsigned long>()->default_value(1635553870000), "Set timestamp from end of run");

    bpo::store(bpo::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
      std::cout << "Please specify:  CalibInputPath :Path to TFile with input histograms: \n mode: time or badchannel \n nameInputHisto: name of input calibration histogram\n";
    }

    if (vm.count("CalibInputPath")) {
      std::cout << "CalibInputPath was set to "
                << vm["CalibInputPath"].as<std::string>() << ".\n";
      CalibInputPath = vm["CalibInputPath"].as<std::string>();
    } else {
      std::cout << "CalibInputPath was not set...\n";
    }

    if (vm.count("ccdbServerPath")) {
      std::cout << "ccdbServerPath was set to "
                << vm["ccdbServerPath"].as<std::string>() << ".\n";
      ccdbServerPath = vm["ccdbServerPath"].as<std::string>();
    } else {
      printf("ccdbServerPath was not set.\nWill use standard path %s", ccdbServerPath.c_str());
    }

    if (vm.count("mode")) {
      std::cout << "mode was set to "
                << vm["mode"].as<std::string>() << ".\n";
      std::string smode = vm["mode"].as<std::string>();
      if (smode.find("time") != std::string::npos) {
        std::cout << "performing time calibration" << std::endl;
        doBadChannelCalib = false;
      } else if (smode.find("badchannel") != std::string::npos) {
        std::cout << "performing bad channel calibration" << std::endl;
        doBadChannelCalib = true;
      } else {
        std::cout << "mode not set... returning\n";
        return 0;
      }
    }

    if (vm.count("nameInputHisto")) {
      std::cout << "nameInputHisto was set to "
                << vm["nameInputHisto"].as<std::string>() << ".\n";
      nameCalibInputHist = vm["nameInputHisto"].as<std::string>();
    }

    if (vm.count("nthreads")) {
      std::cout << "number of threads was set to "
                << vm["nthreads"].as<unsigned int>() << ".\n";
      nthreads = vm["nthreads"].as<unsigned int>();
    }

    if (vm.count("timestampStart")) {
      std::cout << "timestampStart was set to "
                << vm["timestampStart"].as<unsigned long>() << ".\n";
      rangestart = vm["timestampStart"].as<unsigned long>();
    }

    if (vm.count("timestampEnd")) {
      std::cout << "timestampEnd was set to "
                << vm["timestampEnd"].as<unsigned long>() << ".\n";
      rangeend = vm["timestampEnd"].as<unsigned long>();
    }

  } catch (bpo::error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl
              << std::endl;
    std::cerr << opt_general << std::endl;
    exit(1);
  } catch (std::exception& e) {
    std::cerr << e.what() << ", application will now exit" << std::endl;
    exit(2);
  }

  // Set input file and get histogram
  TFile* fTimeCalibInput = TFile::Open(CalibInputPath.c_str());
  if (!fTimeCalibInput) {
    printf("%s not there... returning\n", CalibInputPath.c_str());
    return 0;
  }

  TH2D* hCalibInputHist_ROOT = (TH2D*)fTimeCalibInput->Get(nameCalibInputHist.c_str());
  if (!hCalibInputHist_ROOT) {
    printf("%s not there... returning\n", nameCalibInputHist.c_str());
    return 0;
  }

  // instance of the calib extractor
  o2::emcal::EMCALCalibExtractor CalibExtractor;
  CalibExtractor.setNThreads(nthreads);

  // get boost histo from root input histogram
  auto hCalibInputHist = o2::utils::boostHistoFromRoot_2D(hCalibInputHist_ROOT);

  // instance of CalibDB
  o2::emcal::CalibDB calibdb(ccdbServerPath);

  if (doBadChannelCalib) {
    printf("perform bad channel analysis\n");
    o2::emcal::BadChannelMap BCMap;

    // BCMap = CalibExtractor.calibrateBadChannels(hCalibInputHist);
  } else {
    printf("perform time calibration analysis\n");

    // calibrate the time
    o2::emcal::TimeCalibrationParams TCparams;
    TCparams = CalibExtractor.calibrateTime(hCalibInputHist);

    // store parameters in ccdb via emcal calibdb
    std::map<std::string, std::string> metadata;
    calibdb.storeTimeCalibParam(&TCparams, metadata, rangestart, rangeend);
  }
}
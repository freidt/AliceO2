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

/// @file  InputHelper.cxx

#include "GlobalTrackingWorkflowHelpers/InputHelper.h"
#include "Framework/ConfigParamRegistry.h"
#include "ITSMFTWorkflow/ClusterReaderSpec.h"
#include "ITSWorkflow/TrackReaderSpec.h"
#include "ITSWorkflow/IRFrameReaderSpec.h"
#include "MFTWorkflow/TrackReaderSpec.h"
#include "TPCReaderWorkflow/TrackReaderSpec.h"
#include "TPCReaderWorkflow/ClusterReaderSpec.h"
#include "TPCWorkflow/ClusterSharingMapSpec.h"
#include "GlobalTrackingWorkflowReaders/TrackTPCITSReaderSpec.h"
#include "GlobalTrackingWorkflowReaders/PrimaryVertexReaderSpec.h"
#include "GlobalTrackingWorkflowReaders/SecondaryVertexReaderSpec.h"
#include "GlobalTrackingWorkflowReaders/TrackCosmicsReaderSpec.h"
#include "TOFWorkflowIO/ClusterReaderSpec.h"
#include "TOFWorkflowIO/TOFMatchedReaderSpec.h"
#include "FT0Workflow/RecPointReaderSpec.h"
#include "FV0Workflow/RecPointReaderSpec.h"
#include "FDDWorkflow/RecPointReaderSpec.h"
#include "ZDCWorkflow/RecEventReaderSpec.h"
#include "TRDWorkflowIO/TRDTrackletReaderSpec.h"
#include "TRDWorkflowIO/TRDTrackReaderSpec.h"
#include "MCHWorkflow/TrackReaderSpec.h"

using namespace o2::framework;
using namespace o2::globaltracking;
using namespace o2::dataformats;
using GID = o2::dataformats::GlobalTrackID;

int InputHelper::addInputSpecs(const ConfigContext& configcontext, WorkflowSpec& specs,
                               GID::mask_t maskClusters, GID::mask_t maskMatches, GID::mask_t maskTracks,
                               bool useMC, GID::mask_t maskClustersMC, GID::mask_t maskTracksMC,
                               bool subSpecStrict)
{
  if (configcontext.options().get<bool>("disable-root-input")) {
    return 0;
  }
  if (useMC && configcontext.options().get<bool>("disable-mc")) {
    useMC = false;
  }
  if (!useMC) {
    maskClustersMC = GID::getSourcesMask(GID::NONE);
    maskTracksMC = GID::getSourcesMask(GID::NONE);
  }

  if (maskTracks[GID::ITS]) {
    specs.emplace_back(o2::its::getITSTrackReaderSpec(maskTracksMC[GID::ITS]));
  }
  if (maskClusters[GID::ITS]) {
    specs.emplace_back(o2::itsmft::getITSClusterReaderSpec(maskClustersMC[GID::ITS], true));
  }
  if (maskTracks[GID::MFT]) {
    specs.emplace_back(o2::mft::getMFTTrackReaderSpec(maskTracksMC[GID::MFT]));
  }
  if (maskTracks[GID::MCH]) {
    specs.emplace_back(o2::mch::getTrackReaderSpec(maskTracksMC[GID::MCH]));
  }
  if (maskTracks[GID::TPC]) {
    specs.emplace_back(o2::tpc::getTPCTrackReaderSpec(maskTracksMC[GID::TPC]));
  }
  if (maskClusters[GID::TPC]) {
    specs.emplace_back(o2::tpc::getClusterReaderSpec(maskClustersMC[GID::TPC]));
  }
  if (maskTracks[GID::TPC] && maskClusters[GID::TPC]) {
    specs.emplace_back(o2::tpc::getClusterSharingMapSpec());
  }
  if (maskMatches[GID::ITSTPC] || maskMatches[GID::ITSTPCTOF] || maskTracks[GID::ITSTPC] || maskTracks[GID::ITSTPCTOF]) {
    specs.emplace_back(o2::globaltracking::getTrackTPCITSReaderSpec(maskTracksMC[GID::ITSTPC] || maskTracksMC[GID::ITSTPCTOF]));
  }
  if (maskMatches[GID::ITSTPCTOF] || maskTracks[GID::ITSTPCTOF]) {
    specs.emplace_back(o2::tof::getTOFMatchedReaderSpec(maskTracksMC[GID::ITSTPCTOF], false, /*maskTracks[GID::ITSTPCTOF]*/ false)); // ITSTPCTOF does not provide tracks, only matchInfo
  }
  if (maskClusters[GID::TOF] || maskTracks[GID::ITSTPCTOF]) { // Note: maskTracks[GID::ITSTPCTOF] is only here to match the behavior of RecoContainer::requestTracks
    specs.emplace_back(o2::tof::getClusterReaderSpec(maskClustersMC[GID::TOF]));
  }
  if (maskMatches[GID::TPCTOF]) {
    specs.emplace_back(o2::tof::getTOFMatchedReaderSpec(maskTracksMC[GID::TPCTOF], true, maskTracks[GID::TPCTOF], subSpecStrict));
  }
  if (maskTracks[GID::FT0] || maskClusters[GID::FT0]) {
    specs.emplace_back(o2::ft0::getRecPointReaderSpec(maskTracksMC[GID::FT0] || maskClustersMC[GID::FT0]));
  }
  if (maskTracks[GID::FV0] || maskClusters[GID::FV0]) {
    specs.emplace_back(o2::fv0::getRecPointReaderSpec(maskTracksMC[GID::FV0] || maskClustersMC[GID::FV0]));
  }
  if (maskTracks[GID::FDD] || maskClusters[GID::FDD]) {
    specs.emplace_back(o2::fdd::getFDDRecPointReaderSpec(maskTracksMC[GID::FDD] || maskClustersMC[GID::FDD]));
  }
  if (maskTracks[GID::ZDC] || maskClusters[GID::ZDC]) {
    specs.emplace_back(o2::zdc::getRecEventReaderSpec(maskTracksMC[GID::ZDC] || maskClustersMC[GID::ZDC]));
  }

  if (maskClusters[GID::TRD]) {
    specs.emplace_back(o2::trd::getTRDTrackletReaderSpec(maskClustersMC[GID::TRD], true));
  }
  if (maskTracks[GID::ITSTPCTRD]) {
    specs.emplace_back(o2::trd::getTRDGlobalTrackReaderSpec(maskTracksMC[GID::ITSTPCTRD]));
  }
  if (maskTracks[GID::TPCTRD]) {
    specs.emplace_back(o2::trd::getTRDTPCTrackReaderSpec(maskTracksMC[GID::TPCTRD], subSpecStrict));
  }

  return 0;
}

// attach primary vertex reader
int InputHelper::addInputSpecsPVertex(const o2::framework::ConfigContext& configcontext, o2::framework::WorkflowSpec& specs, bool mc)
{
  if (configcontext.options().get<bool>("disable-root-input")) {
    return 0;
  }
  specs.emplace_back(o2::vertexing::getPrimaryVertexReaderSpec(mc));
  return 0;
}

// attach secondary vertex reader
int InputHelper::addInputSpecsSVertex(const o2::framework::ConfigContext& configcontext, o2::framework::WorkflowSpec& specs)
{
  if (configcontext.options().get<bool>("disable-root-input")) {
    return 0;
  }
  specs.emplace_back(o2::vertexing::getSecondaryVertexReaderSpec());
  return 0;
}

// attach cosmic tracks reader
int InputHelper::addInputSpecsCosmics(const o2::framework::ConfigContext& configcontext, o2::framework::WorkflowSpec& specs, bool mc)
{
  if (configcontext.options().get<bool>("disable-root-input")) {
    return 0;
  }
  specs.emplace_back(o2::globaltracking::getTrackCosmicsReaderSpec(mc));
  return 0;
}

// attach vector of ITS reconstructed IRFrames
int InputHelper::addInputSpecsIRFramesITS(const o2::framework::ConfigContext& configcontext, o2::framework::WorkflowSpec& specs)
{
  if (configcontext.options().get<bool>("disable-root-input")) {
    return 0;
  }
  specs.emplace_back(o2::its::getIRFrameReaderSpec());
  return 0;
}

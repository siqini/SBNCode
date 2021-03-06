#include <iostream>
#include <vector>
#include <TH2D.h>
#include <json/json.h>
#include <cmath>
#include "gallery/ValidHandle.h"
#include "canvas/Utilities/InputTag.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "nusimdata/SimulationBase/MCNeutrino.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "lardataobj/MCBase/MCTrack.h"
#include "lardataobj/MCBase/MCShower.h"
#include "lardataobj/MCBase/MCStep.h"
#include "gallery/Event.h"
#include <TLorentzVector.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <random>
#include "core/Event.hh"
#include "NueSelection.h"
#include "Utilities.h"

namespace ana {
  namespace SBNOsc {

NueSelection::NueSelection() : SelectionBase(), fEventCounter(0), fNuCount(0){}


void NueSelection::Initialize(Json::Value* config) {

  fDiffLength = new TH1D ("diff_length","",200,0,200);
  fMatchedNuHist = new TH1D("matched_nu_hist","",60,0,6);
  fShowerEvDiffLength = new TH2D("shower_e_v_diff_length","",200,0,200,100,0,10);
  fEnuEeHist = new TH2D("EnuEeHist","",6000,0,6000,6000,0,6000);
  // Load configuration parameters
  fTruthTag = { "generator" };
  fTrackTag = { "mcreco" };
  fShowerTag = { "mcreco" };

  if (config) {
    fTruthTag = { (*config)["SBNOsc"].get("MCTruthTag", "generator").asString() };
    fTrackTag = { (*config)["SBNOsc"].get("MCTrackTag", "mcreco").asString() };
    fShowerTag = { (*config)["SBNOsc"].get("MCShowerTag","mcreco").asString() };
  }

  AddBranch("nucount",&fNuCount);


  hello();
}


void NueSelection::Finalize() {
  fOutputFile->cd();
  fDiffLength->Write();
  fMatchedNuHist->Write();
  fShowerEvDiffLength->Write();
  fEnuEeHist->Write();
}



bool NueSelection::ProcessEvent(const gallery::Event& ev, std::vector<Event::Interaction>& reco) {
  if (fEventCounter % 10 == 0) {
    std::cout << "NueSelection: Processing event " << fEventCounter << " "
              << "(" << fNuCount << " neutrinos selected)"
              << std::endl;
  }
  fEventCounter++;

  // Grab data products from the event
  auto const& mctruths = \
    *ev.getValidHandle<std::vector<simb::MCTruth> >(fTruthTag);
  auto const& mctracks = \
    *ev.getValidHandle<std::vector<sim::MCTrack> >(fTrackTag);
  auto const& mcshowers = \
    *ev.getValidHandle<std::vector<sim::MCShower> >(fShowerTag);

  std::vector<bool> matchedness;
  std::vector<double> assnee;
  // Iterate through the neutrinos
  for (size_t i=0;i<mctruths.size();i++) {
    auto const& mctruth = mctruths.at(i);
    const simb::MCNeutrino& nu = mctruth.GetNeutrino();
    auto nu_pos = nu.Nu().Position();
    int matched_shower_count = 0;
    std::vector<double> ees;
    for (size_t j=0;j<mcshowers.size();j++) {
      auto const& mcshower = mcshowers.at(j);
      auto shower_pos = mcshower.DetProfile().Position();
      auto shower_E = mcshower.DetProfile().E();
      double distance = (nu_pos.Vect()-shower_pos.Vect()).Mag();
      fDiffLength->Fill(distance);
      fShowerEvDiffLength->Fill(distance,shower_E);
      if (distance <= 5.) {
        matched_shower_count++;
        ees.push_back(shower_E);
      }
    }
    if (matched_shower_count>0) {
      matchedness.push_back(true);
      assnee.push_back(ees[0]);
    }
    else matchedness.push_back(false);



  // Iterate through the neutrinos/MCTruth
  for (size_t i=0; i<mctruths.size(); i++) {
    Event::Interaction interaction;
    auto const& mctruth = mctruths.at(i);
    const simb::MCNeutrino& nu = mctruth.GetNeutrino();
    auto nu_E = nu.Nu().E();
    if (matchedness[i]==true) {
      fMatchedNuHist->Fill(nu_E);
      fEnuEeHist->Fill(assnee[i],nu_E*1000);
      Event::Interaction interaction = TruthReco(mctruth);
      reco.push_back(interaction);
    }
    /*
    if (nu.CCNC() == simb::kCC && nu.Mode() == 0 && nu.Nu().PdgCode() == 12) {
      Event::Interaction interaction = TruthReco(mctruth);
      reco.push_back(interaction);
    }
    */
  }

  bool selected = !reco.empty(); // true if reco info is not empty

  if (selected) {
    fNuCount++;
  }

  return selected;
}

  }  // namespace SBNOsc
}  // namespace ana


DECLARE_SBN_PROCESSOR(ana::SBNOsc::NueSelection)



#include "TracksInJets.h"

#include <trackbase_historic/SvtxTrack_v4.h>
#include <trackbase_historic/SvtxTrackMap.h>

#include <trackbase_historic/SvtxTrackMap_v2.h>

#include <globalvertex/SvtxVertexMap.h>
#include <globalvertex/GlobalVertex.h>
#include <globalvertex/GlobalVertexMap.h>
#include <globalvertex/GlobalVertexMapv1.h>


#include <g4eval/SvtxEvalStack.h>
#include <sstream>

#include <jetbase/JetMap.h>
#include <jetbase/JetContainer.h>
#include <jetbase/Jetv2.h>
#include <jetbase/Jetv1.h>
#include <jetbase/Jet.h>
#include <jetbase/JetAlgo.h>
#include <jetbase/FastJetAlgo.h>
#include <jetbase/JetInput.h>
#include <jetbase/TowerJetInput.h>
#include <jetbase/JetMap.h>
#include <jetbase/JetMapv1.h>
#include <jetbase/JetContainer.h>
#include <jetbase/JetContainerv1.h>
#include <centrality/CentralityInfo.h>

#include <fun4all/Fun4AllBase.h>
#include <fun4all/Fun4AllReturnCodes.h>
#include <fun4all/PHTFileServer.h>

#include <phool/PHCompositeNode.h>
#include <phool/getClass.h>


#include "TH1.h"
#include "TH2.h"
#include <TTree.h>
#include <TFile.h>
#include <TH3F.h>
#include <TH2F.h>
#include <TString.h>
#include <TVector3.h>
#include <TTree.h>
#include <cassert>

//____________________________________________________________________________..
TracksInJets::TracksInJets(const std::string& recojetname, const std::string& outputfilename):
  SubsysReco("TracksInJets_" + recojetname)
  , m_recoJetName(recojetname)
  , m_trk_pt_cut(2)
  , m_jetRadius(0.4)
  , m_outputFileName(outputfilename)
{
  std::cout << "TracksInJets::TracksInJets(const std::string &name) Calling ctor" << std::endl;
}

//____________________________________________________________________________..
TracksInJets::~TracksInJets()
{
  std::cout << "TracksInJets::~TracksInJets() Calling dtor" << std::endl;
}

//____________________________________________________________________________..
int TracksInJets::Init(PHCompositeNode *topNode)
{
  std::cout << "TracksInJets::Init(PHCompositeNode *topNode) Initializing" << std::endl;
  PHTFileServer::get().open(m_outputFileName, "RECREATE");
  m_h_track_vs_calo_pt = new TH3F("m_h_track_vs_calo_pt","",100,0,100,500,0,100,10,0,100);
  m_h_track_vs_calo_pt->GetXaxis()->SetTitle("Jet p_{T} [GeV]");
  m_h_track_vs_calo_pt->GetYaxis()->SetTitle("Sum track p_{T} [GeV]");
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int TracksInJets::InitRun(PHCompositeNode *topNode)
{
  std::cout << "TracksInJets::InitRun(PHCompositeNode *topNode) Initializing for Run XXX" << std::endl;
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int TracksInJets::process_event(PHCompositeNode *topNode)
{
  //std::cout << "TracksInJets::process_event(PHCompositeNode *topNode) Processing Event" << std::endl;
 
 // interface to reco jets
  JetContainer* jets = findNode::getClass<JetContainer>(topNode, m_recoJetName);
  if (!jets)
    {
      std::cout
	<< "MyJetAnalysis::process_event - Error can not find DST Reco JetContainer node "
	<< m_recoJetName << std::endl;
      exit(-1);
    }
  //get reco tracks
  SvtxTrackMap* trackmap = findNode::getClass<SvtxTrackMap>(topNode, "SvtxTrackMap");
  if (!trackmap)
    {
      trackmap = findNode::getClass<SvtxTrackMap>(topNode, "TrackMap");
      if (!trackmap)
	{
	  std::cout
	    << "TracksInJets::process_event - Error can not find DST trackmap node SvtxTrackMap" << std::endl;
	  exit(-1);
	}
    }

  //get event centrality
  CentralityInfo* cent_node = findNode::getClass<CentralityInfo>(topNode, "CentralityInfo");
  if (!cent_node)
    {
      std::cout
        << "TracksInJets::process_event - Error can not find centrality node "
        << std::endl;
      exit(-1);
    }
  int cent = cent_node->get_centile(CentralityInfo::PROP::mbd_NS);


  // Loop through jets
  for(auto jet : *jets)
    {
      if(!jet)
	{
	  std::cout << "WARNING!!! Jet not found" << std::endl;
	  continue;
	}
      //sum up tracks in jet
      TVector3 sumtrk(0, 0, 0);
      for (SvtxTrackMap::Iter iter = trackmap->begin();
	   iter != trackmap->end();
	   ++iter)
	{
	  SvtxTrack* track = iter->second;
	  float quality = track->get_quality();
	  auto silicon_seed = track->get_silicon_seed();
	  int nmvtxhits = 0;
	  if (silicon_seed)
	    {
	      nmvtxhits = silicon_seed->size_cluster_keys();
	    }
	  if(track->get_pt() < m_trk_pt_cut || quality > 6 || nmvtxhits < 4) //do some basic quality selections on tracks
	    {
	      continue;
	    }

      // Calculate delta eta, delta phi, and dR
      TVector3 v(track->get_px(), track->get_py(), track->get_pz());
      double dEta = v.Eta() - jet->get_eta();
      double dPhi = v.Phi() - jet->get_phi();
      while (dPhi > M_PI) dPhi -= 2 * M_PI;
      while (dPhi < -M_PI) dPhi += 2 * M_PI;
      double dR = sqrt(dEta * dEta + dPhi * dPhi);

      // Check if track is within jet radius
      if (dR < m_jetRadius) {
	sumtrk += v;
      }
    }

    // Fill histogram for the current jet
    assert(m_h_track_vs_calo_pt);
    m_h_track_vs_calo_pt->Fill(jet->get_pt(), sumtrk.Perp(), cent);

    // Reset sumtrk for the next jet
    sumtrk.SetXYZ(0, 0, 0);
  }
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int TracksInJets::ResetEvent(PHCompositeNode *topNode)
{
  //std::cout << "TracksInJets::ResetEvent(PHCompositeNode *topNode) Resetting internal structures, prepare for next event" << std::endl;
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int TracksInJets::EndRun(const int runnumber)
{
  std::cout << "TracksInJets::EndRun(const int runnumber) Ending Run for Run " << runnumber << std::endl;
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int TracksInJets::End(PHCompositeNode *topNode)
{
  std::cout << "TracksInJets::End(PHCompositeNode *topNode) This is the End..." << std::endl;
  PHTFileServer::get().cd(m_outputFileName);

  TH2 *h_proj;
  for(int i = 0; i < m_h_track_vs_calo_pt->GetNbinsZ(); i++)
    {
      m_h_track_vs_calo_pt->GetZaxis()->SetRange(i+1,i+1);
      h_proj = (TH2*) m_h_track_vs_calo_pt->Project3D("yx");
      h_proj->Write(Form("h_track_vs_calo_%1.0f",m_h_track_vs_calo_pt->GetZaxis()->GetBinLowEdge(i+1)));
    }

  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int TracksInJets::Reset(PHCompositeNode *topNode)
{
  std::cout << "TracksInJets::Reset(PHCompositeNode *topNode) being Reset" << std::endl;
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
void TracksInJets::Print(const std::string &what) const
{
  std::cout << "TracksInJets::Print(const std::string &what) const Printing info for " << what << std::endl;
}

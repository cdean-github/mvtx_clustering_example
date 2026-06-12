#include "silicon_detector_analyser.h"

#include <fun4all/Fun4AllReturnCodes.h>

#include <phool/getClass.h>
#include <phool/PHCompositeNode.h>
#include <phool/PHNodeIterator.h>
#include <phool/recoConsts.h>

#include <boost/format.hpp>
#include <boost/math/special_functions/sign.hpp>
// Reference: https://github.com/sPHENIX-Collaboration/analysis/blob/master/TPC/DAQ/macros/prelimEvtDisplay/TPCEventDisplay.C
//____________________________________________________________________________..
silicon_detector_analyser::silicon_detector_analyser(const std::string &name):
 SubsysReco(name)
{
}

//____________________________________________________________________________..
silicon_detector_analyser::~silicon_detector_analyser()
{
}

//____________________________________________________________________________..
int silicon_detector_analyser::Init(PHCompositeNode *topNode)
{
  outRoot = new TFile(outRootName.c_str(), "RECREATE");
  outTree = new TTree("Clusters", "Clusters");
  outTree->OptimizeBaskets();
  outTree->SetAutoSave(-5e6);

  outTree->Branch("event", &event, "event/I");
  outTree->Branch("BCO", &triggerBCO, "triggerBCO/l");
  outTree->Branch("vertex_x", &vertex_x, "vertex_x/F");
  outTree->Branch("vertex_y", &vertex_y, "vertex_y/F");
  outTree->Branch("vertex_z", &vertex_z, "vertex_z/F");
  outTree->Branch("clusLayer", &clusLayer);
  outTree->Branch("clusPhi", &clusPhi);
  outTree->Branch("clusEta", &clusEta);
  outTree->Branch("clusSize", &clusSize);
  outTree->Branch("clusX", &clusX);
  outTree->Branch("clusY", &clusY);
  outTree->Branch("clusZ", &clusZ);

  return Fun4AllReturnCodes::EVENT_OK;
}
//____________________________________________________________________________..
int silicon_detector_analyser::process_event(PHCompositeNode *topNode)
{
  ++event;
  PHNodeIterator dstiter(topNode);

  recoConsts *rc = recoConsts::instance();
  m_runNumber = rc->get_IntFlag("RUNNUMBER");

  PHCompositeNode* dstNode = dynamic_cast<PHCompositeNode *>(dstiter.findFirst("PHCompositeNode", "DST"));
  if (!dstNode)
  {
    std::cout << __FILE__ << "::" << __func__ << " - DST Node missing, doing nothing." << std::endl;
    exit(1);
  }

  trkrHitSetContainer = findNode::getClass<TrkrHitSetContainer>(dstNode, "TRKR_HITSET");
  if (!trkrHitSetContainer)
  {
    std::cout << __FILE__ << "::" << __func__ << " - TRKR_HITSET  missing, doing nothing." << std::endl;
    exit(1);
  }

  trktClusterContainer = findNode::getClass<TrkrClusterContainer>(dstNode, "TRKR_CLUSTER");
  if (!trktClusterContainer)
  {
    std::cout << __FILE__ << "::" << __func__ << " - TRKR_CLUSTER missing, doing nothing." << std::endl;
    exit(1);
  }

  actsGeom = findNode::getClass<ActsGeometry>(topNode, "ActsGeometry");
  if (!actsGeom)
  {
    std::cout << __FILE__ << "::" << __func__ << " - ActsGeometry missing, doing nothing." << std::endl;
    exit(1);
  }

  mvtx_event_header = findNode::getClass<MvtxEventInfo>(topNode, "MVTXEVENTHEADER");
  if (!mvtx_event_header)
  {
    std::cout << __FILE__ << "::" << __func__ << " - MVTXEVENTHEADER missing, doing nothing." << std::endl;
    exit(1);
  }

  std::set<uint64_t> strobeList = mvtx_event_header->get_strobe_BCOs();
  for (auto iterStrobe = strobeList.begin(); iterStrobe != strobeList.end(); ++iterStrobe)
  {
    std::set<uint64_t> l1List = mvtx_event_header->get_L1_BCO_from_strobe_BCO(*iterStrobe);
    for (auto iterL1 = l1List.begin(); iterL1 != l1List.end(); ++iterL1)
    {
      L1_BCOs.push_back(*iterL1); 
    }
  }

  SvtxTrackMap* trackMap = findNode::getClass<SvtxTrackMap>(topNode, "SvtxTrackMap");
  if (!trackMap)
  {
    std::cout << __FILE__ << "::" << __func__ << " - SvtxTrackMap missing, doing nothing." << std::endl;
    exit(1);
  }

  SvtxVertexMap* vertexMap = findNode::getClass<SvtxVertexMap>(topNode, "SvtxVertexMap");
  if (!vertexMap)
  {
    std::cout << __FILE__ << "::" << __func__ << " - SvtxVertexMap missing, doing nothing." << std::endl;
    exit(1);
  }

  numberL1s = mvtx_event_header->get_number_L1s();
  layer = 0;
  clusLayer.clear();
  clusPhi.clear();
  clusEta.clear();
  clusSize.clear();
  clusX.clear();
  clusY.clear();
  clusZ.clear();

  SvtxVertex* thePV = vertexMap->begin()->second;
  vertex_x = thePV->get_x();
  vertex_y = thePV->get_y();
  vertex_z = thePV->get_z();

  triggerBCO = L1_BCOs.size() > 0 ? L1_BCOs[0] : event;
  for (auto& iter : *trackMap)
  {
    TrackSeed* silseed = iter.second->get_silicon_seed(); 
    TVector2 LocalUse;
    TVector3 ClusterWorld;

    for (SvtxTrack::ConstClusterKeyIter iter_local = silseed->begin_cluster_keys();
     iter_local != silseed->end_cluster_keys();
     ++iter_local)
    {
      TrkrDefs::cluskey cluster_key = *iter_local;
      layer = TrkrDefs::getLayer(cluster_key);

      TrkrCluster *cluster = trktClusterContainer->findCluster(cluster_key);

      localX = cluster->getLocalX();
      localY = cluster->getLocalY();

      LocalUse.SetX(localX);
      LocalUse.SetY(localY);

      auto surface = actsGeom->maps().getSurface(cluster_key, cluster);

      switch (TrkrDefs::getTrkrId(cluster_key))
      {
        case TrkrDefs::TrkrId::mvtxId:
        {
          ClusterWorld = CylinderGeom_MvtxHelper::get_world_from_local_coords(surface, actsGeom, LocalUse);
          break;
        }
        case TrkrDefs::TrkrId::inttId:
        {
          ClusterWorld = CylinderGeomInttHelper::get_world_from_local_coords(surface, actsGeom, LocalUse);
          break;
        }
        default:
          break;
      }

      clusLayer.push_back(layer);
      clusPhi.push_back(iter.second->get_phi());
      clusEta.push_back(iter.second->get_eta());
      clusSize.push_back(cluster->getAdc());
      clusX.push_back(ClusterWorld.X());
      clusY.push_back(ClusterWorld.Y());
      clusZ.push_back(ClusterWorld.Z());
    }
  }

  outTree->Fill();

  L1_BCOs.clear();

  return Fun4AllReturnCodes::EVENT_OK;
}


//____________________________________________________________________________..
int silicon_detector_analyser::End(PHCompositeNode *topNode)
{
  outRoot->Write();
  outRoot->Close();
  delete outRoot;

  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int silicon_detector_analyser::Reset(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
void silicon_detector_analyser::Print(const std::string &what) const
{
}

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
  outTree->Branch("clusLayer", &clusLayer);
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

  numberL1s = mvtx_event_header->get_number_L1s();
  layer = 0;
  clusLayer.clear();
  clusSize.clear();
  clusX.clear();
  clusY.clear();
  clusZ.clear();

  triggerBCO = L1_BCOs.size() > 0 ? L1_BCOs[0] : event;
  TVector2 LocalUse;
  TVector3 ClusterWorld;

  TrkrHitSetContainer::ConstRange hitsetrange = trkrHitSetContainer->getHitSets(TrkrDefs::TrkrId::mvtxId);

  for (TrkrHitSetContainer::ConstIterator hitsetitr = hitsetrange.first; hitsetitr != hitsetrange.second; ++hitsetitr)
  {
    TrkrClusterContainer::ConstRange clusterrange = trktClusterContainer->getClusters(hitsetitr->first);

    for (TrkrClusterContainer::ConstIterator clusteritr = clusterrange.first; clusteritr != clusterrange.second; ++clusteritr)
    {
      TrkrDefs::cluskey cluster_key = clusteritr->first;
      TrkrCluster *cluster = clusteritr->second;
      layer = TrkrDefs::getLayer(cluster_key);

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

//____________________________________________________________________________..
//
// This is a template for a Fun4All SubsysReco module with all methods from the
// $OFFLINE_MAIN/include/fun4all/SubsysReco.h baseclass
// You do not have to implement all of them, you can just remove unused methods
// here and in VertexCompare.h.
//
// VertexCompare(const std::string &name = "VertexCompare")
// everything is keyed to VertexCompare, duplicate names do work but it makes
// e.g. finding culprits in logs difficult or getting a pointer to the module
// from the command line
//
// VertexCompare::~VertexCompare()
// this is called when the Fun4AllServer is deleted at the end of running. Be
// mindful what you delete - you do loose ownership of object you put on the node tree
//
// int VertexCompare::Init(PHCompositeNode *topNode)
// This method is called when the module is registered with the Fun4AllServer. You
// can create historgrams here or put objects on the node tree but be aware that
// modules which haven't been registered yet did not put antyhing on the node tree
//
// int VertexCompare::InitRun(PHCompositeNode *topNode)
// This method is called when the first event is read (or generated). At
// this point the run number is known (which is mainly interesting for raw data
// processing). Also all objects are on the node tree in case your module's action
// depends on what else is around. Last chance to put nodes under the DST Node
// We mix events during readback if branches are added after the first event
//
// int VertexCompare::process_event(PHCompositeNode *topNode)
// called for every event. Return codes trigger actions, you find them in
// $OFFLINE_MAIN/include/fun4all/Fun4AllReturnCodes.h
//   everything is good:
//     return Fun4AllReturnCodes::EVENT_OK
//   abort event reconstruction, clear everything and process next event:
//     return Fun4AllReturnCodes::ABORT_EVENT; 
//   proceed but do not save this event in output (needs output manager setting):
//     return Fun4AllReturnCodes::DISCARD_EVENT; 
//   abort processing:
//     return Fun4AllReturnCodes::ABORT_RUN
// all other integers will lead to an error and abort of processing
//
// int VertexCompare::ResetEvent(PHCompositeNode *topNode)
// If you have internal data structures (arrays, stl containers) which needs clearing
// after each event, this is the place to do that. The nodes under the DST node are cleared
// by the framework
//
// int VertexCompare::EndRun(const int runnumber)
// This method is called at the end of a run when an event from a new run is
// encountered. Useful when analyzing multiple runs (raw data). Also called at
// the end of processing (before the End() method)
//
// int VertexCompare::End(PHCompositeNode *topNode)
// This is called at the end of processing. It needs to be called by the macro
// by Fun4AllServer::End(), so do not forget this in your macro
//
// int VertexCompare::Reset(PHCompositeNode *topNode)
// not really used - it is called before the dtor is called
//
// void VertexCompare::Print(const std::string &what) const
// Called from the command line - useful to print information when you need it
//
//____________________________________________________________________________..

#include "VertexCompare.h"

#include <fun4all/Fun4AllReturnCodes.h>

#include <phool/PHCompositeNode.h>
#include <phool/getClass.h>

#include <ffarawobjects/Gl1Packet.h>

#include <globalvertex/GlobalVertex.h>
#include <globalvertex/GlobalVertexMap.h>
#include <globalvertex/MbdVertex.h>
#include <globalvertex/MbdVertexMap.h>
#include <globalvertex/SvtxVertex.h>
#include <globalvertex/SvtxVertexMap.h>

GlobalVertex::VTXTYPE trkType = GlobalVertex::SVTX;
GlobalVertex::VTXTYPE mbdType = GlobalVertex::MBD;
//____________________________________________________________________________..
VertexCompare::VertexCompare(const std::string &name):
 SubsysReco(name)
{
  std::cout << "VertexCompare::VertexCompare(const std::string &name) Calling ctor" << std::endl;
}

//____________________________________________________________________________..
VertexCompare::~VertexCompare()
{
  std::cout << "VertexCompare::~VertexCompare() Calling dtor" << std::endl;
}

//____________________________________________________________________________..
int VertexCompare::Init(PHCompositeNode *topNode)
{
  outFile = new TFile(outFileName.c_str(), "RECREATE");
  outTree = new TTree("VTX", "VTX");
  outTree->OptimizeBaskets();
  outTree->SetAutoSave(-5e6);

  outTree->Branch("counter", &counter, "counter/I");
  outTree->Branch("mbdVertex", &mbdVertex, "mbdVertex/F");
  outTree->Branch("trackerVertex", &trackerVertex, "trackerVertex/F");
  outTree->Branch("nTracks", &nTracks, "nTracks/i");
  outTree->Branch("n_MBDVertex", &n_MBDVertex, "n_MBDVertex/i");
  outTree->Branch("n_TRKVertex", &n_TRKVertex, "n_TRKVertex/i");
  outTree->Branch("hasMBD", &hasMBD, "hasMBD/O");
  outTree->Branch("hasTRK", &hasTRK, "hasTRK/O");

  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int VertexCompare::InitRun(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int VertexCompare::process_event(PHCompositeNode *topNode)
{
  MbdVertexMap *m_dst_mbdvertexmap = findNode::getClass<MbdVertexMap>(topNode, "MbdVertexMap");
  SvtxVertexMap *m_dst_vertexmap = findNode::getClass<SvtxVertexMap>(topNode, "SvtxVertexMap");

  auto globalvertexmap = findNode::getClass<GlobalVertexMap>(topNode, "GlobalVertexMap");

  mbdVertex = trackerVertex = std::numeric_limits<float>::quiet_NaN();
  nTracks = n_MBDVertex = n_TRKVertex = std::numeric_limits<unsigned int>::quiet_NaN();

  hasMBD = false;
  hasTRK = false;

  for (GlobalVertexMap::ConstIter iter = globalvertexmap->begin(); iter != globalvertexmap->end(); ++iter)
  {
    GlobalVertex *gvertex = iter->second;

    if (gvertex->count_vtxs(mbdType) != 0)
    {
      hasMBD = true;

      auto mbditer = gvertex->find_vertexes(mbdType);
      auto mbdvertexvector = mbditer->second;

      n_MBDVertex = mbdvertexvector.size();
      for (auto &vertex : mbdvertexvector)
      {
        MbdVertex *m_dst_vertex = m_dst_mbdvertexmap->find(vertex->get_id())->second;
        mbdVertex = m_dst_vertex->get_z();
      }
    }

    if (gvertex->count_vtxs(trkType) != 0)
    { 
      hasTRK = true; 

      auto trkiter = gvertex->find_vertexes(trkType);
      auto trkvertexvector = trkiter->second;

      n_TRKVertex = trkvertexvector.size();
      for (auto &vertex : trkvertexvector)
      {
        SvtxVertex *m_dst_vertex = m_dst_vertexmap->find(vertex->get_id())->second;
        if ( m_dst_vertex->get_beam_crossing() != 0 ) continue; 
        if ( m_dst_vertex->size_tracks() > nTracks)
        {
          trackerVertex = m_dst_vertex->get_z();
          nTracks = m_dst_vertex->size_tracks();
        }
        if (nTracks == 0) hasTRK = false; 
      }
    }
  }

  outTree->Fill();

  ++counter;

  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int VertexCompare::ResetEvent(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int VertexCompare::EndRun(const int runnumber)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int VertexCompare::End(PHCompositeNode *topNode)
{
  outFile->Write();
  outFile->Close();
  delete outFile;

  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int VertexCompare::Reset(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
void VertexCompare::Print(const std::string &what) const
{
}

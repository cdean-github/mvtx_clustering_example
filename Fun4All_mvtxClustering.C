#include <fun4all/Fun4AllDstOutputManager.h>
#include <fun4all/Fun4AllInputManager.h>
#include <fun4all/Fun4AllOutputManager.h>
#include <fun4all/Fun4AllRunNodeInputManager.h>
#include <fun4all/Fun4AllServer.h>
#include <fun4allraw/Fun4AllEventOutputManager.h>
#include <fun4allraw/Fun4AllPrdfInputManager.h>
#include <fun4allraw/Fun4AllStreamingInputManager.h>
#include <fun4allraw/InputManagerType.h>
#include <fun4allraw/SingleInttPoolInput.h>
#include <fun4allraw/SingleGl1PoolInput.h>
#include <fun4allraw/SingleMvtxPoolInput.h>

#include <ffamodules/CDBInterface.h>
#include <ffamodules/FlagHandler.h>
#include <ffamodules/HeadReco.h>
#include <ffamodules/SyncReco.h>

#include <cdbobjects/CDBTTree.h>
#include <phool/recoConsts.h>

#include <GlobalVariables.C>
#include <G4_ActsGeom.C>
#include <Trkr_Clustering.C>
#include <Trkr_Reco.C>
#include <Trkr_RecoInit.C>

#include <string>

#include <vertexcompare/silicon_detector_analyser.h>

R__LOAD_LIBRARY(libmbd.so)
R__LOAD_LIBRARY(libglobalvertex.so)
R__LOAD_LIBRARY(libVertexCompare.so)
R__LOAD_LIBRARY(libfun4all.so)
R__LOAD_LIBRARY(libfun4allraw.so)
R__LOAD_LIBRARY(libffamodules.so)
R__LOAD_LIBRARY(libffarawmodules.so)

bool isGood(const string &infile)
{
  ifstream intest;
  intest.open(infile);
  bool goodfile = false;
  if (intest.is_open())
  {
    if (intest.peek() != std::ifstream::traits_type::eof())  // is it non zero?
    {
      goodfile = true;
    }
    intest.close();
  }
  return goodfile;
}

void Fun4All_mvtxClustering(string runNumber = "68626", string outputDir = "", const int nEvents = 3e3)
{
  Fun4AllServer *se = Fun4AllServer::instance();
  se->Verbosity(1);

  Enable::CDB = true;
  auto rc = recoConsts::instance();
  rc->set_IntFlag("RUNNUMBER", i_runNumber);
  rc->set_StringFlag("CDB_GLOBALTAG", "ProdA_2024");
  rc->set_uint64Flag("TIMESTAMP", i_runNumber);

  std::string geofile = CDBInterface::instance()->getUrl("Tracking_Geometry");
  Fun4AllRunNodeInputManager *ingeo = new Fun4AllRunNodeInputManager("GeoIn");
  ingeo->AddFile(geofile);
  string gl1input = outputDir + "/lists/gl1daq.list";
  vector<string> mvtxinput;

  for (unsigned int i = 0; i < 6; ++i) mvtxinput.push_back(outputDir + "/lists/mvtx_flx" + to_string(i) + ".list");

  Enable::CDB = true;

  Fun4AllStreamingInputManager *inStream = new Fun4AllStreamingInputManager("Comb");

  int NumInputs = 0;
  ifstream infile(gl1input);
  if (infile.is_open())
  {
    SingleGl1PoolInput *gl1_sngl = new SingleGl1PoolInput("GL1");
    gl1_sngl->Verbosity(0);
    gl1_sngl->AddListFile(gl1input);
    inStream->registerStreamingInput(gl1_sngl, InputManagerType::GL1);
    infile.close();
    NumInputs++;
  }

  int i = 0;
  for (auto& file : mvtxinput)
  {
    auto* sngl= new SingleMvtxPoolInput("MVTX_FLX" + to_string(i));
    sngl->Verbosity(0);
    sngl->SetBcoRange(200);
    sngl->SetNegativeBco(100);
    sngl->AddListFile(file);
    inStream->registerStreamingInput(sngl, InputManagerType::MVTX);
    NumInputs++;
    i++;
  }

  se->registerInputManager(inStream);

  // if there is no input manager this macro will still run - so just quit here
  if (NumInputs == 0)
  {
    std::cout << "no file lists no input manager registered, quitting" << std::endl;
    gSystem->Exit(1);
  }

  SyncReco *sync = new SyncReco();
  se->registerSubsystem(sync);

  HeadReco *head = new HeadReco();
  se->registerSubsystem(head);

  FlagHandler *flag = new FlagHandler();
  se->registerSubsystem(flag);

  Enable::MVTX_APPLYMISALIGNMENT = true;
  ACTSGEOM::mvtx_applymisalignment = Enable::MVTX_APPLYMISALIGNMENT;

  se->registerInputManager(ingeo);

  TrackingInit();

  Mvtx_HitUnpacking();

  Mvtx_Clustering();

  silicon_detector_analyser cluster_analyser = new silicon_detector_analyser();
  se->registerSubsystem(cluster_analyser);

  se->run(nEvents);
  se->End();
  delete se;
}

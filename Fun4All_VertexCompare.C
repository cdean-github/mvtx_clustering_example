#include <fun4all/Fun4AllDstOutputManager.h>
#include <fun4all/Fun4AllInputManager.h>
#include <fun4all/Fun4AllOutputManager.h>
#include <fun4all/Fun4AllRunNodeInputManager.h>
#include <fun4all/Fun4AllServer.h>
#include <fun4allraw/Fun4AllEventOutputManager.h>
#include <fun4allraw/Fun4AllPrdfInputManager.h>
#include <fun4allraw/Fun4AllStreamingInputManager.h>
#include <fun4allraw/Fun4AllTriggeredInputManager.h>
#include <fun4allraw/InputManagerType.h>
#include <fun4allraw/SingleInttPoolInput.h>
#include <fun4allraw/SingleGl1PoolInput.h>
#include <fun4allraw/SingleGl1TriggeredInput.h>
#include <fun4allraw/SingleMvtxPoolInput.h>
#include <fun4allraw/SingleTriggeredInput.h>

#include <ffamodules/CDBInterface.h>
#include <ffamodules/FlagHandler.h>
#include <ffamodules/HeadReco.h>
#include <ffamodules/SyncReco.h>

#include <cdbobjects/CDBTTree.h>
#include <phool/recoConsts.h>

#include <globalvertex/GlobalVertexReco.h>
#include <mbd/MbdReco.h>
#include <GlobalVariables.C>
#include <G4_ActsGeom.C>
#include <Trkr_Clustering.C>
#include <Trkr_Reco.C>
#include <Trkr_RecoInit.C>

#include <string>

#include <vertexcompare/VertexCompare.h>

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

string Fun4All_MBD_data(const int nEvents = 3e3, const int runNumber = 68626, const string outputDir = "dummyDir")
{
  string gl1Input = outputDir + "/lists/gl1daq.list";
  string mbdInput = outputDir + "/lists/seb18.list";
  string dstOutput = outputDir + "/files/DST_MBD.root"; 

  Fun4AllServer *se = Fun4AllServer::instance();
  se->Verbosity(1);

  Fun4AllTriggeredInputManager *in = new Fun4AllTriggeredInputManager("Comb");

  int numInputs = 0;
  ifstream infile(gl1Input);
  if (infile.is_open())
  {
    infile.close();
    SingleTriggeredInput *gl1 = new SingleGl1TriggeredInput("Gl1in");
    gl1->KeepPackets();
    gl1->AddListFile(gl1Input);
    in->registerGl1TriggeredInput(gl1);
    numInputs++;
  }

  infile.open(mbdInput);
  if (infile.is_open())
  {
    infile.close();
    SingleTriggeredInput *mbd = new SingleTriggeredInput("Mbdin");
    mbd->AddListFile(mbdInput);
    in->registerTriggeredInput(mbd);
    numInputs++;
  }

  if (numInputs == 0)
  {
    std::cout << "no file lists no input manager registered, quitting" << std::endl;
    gSystem->Exit(1);
  }
  se->registerInputManager(in);

  SyncReco *sync = new SyncReco();
  se->registerSubsystem(sync);

  HeadReco *head = new HeadReco();
  se->registerSubsystem(head);

  FlagHandler *flag = new FlagHandler();
  se->registerSubsystem(flag);

  MbdReco *mbdreco = new MbdReco();
  mbdreco->Verbosity(0);
  se->registerSubsystem(mbdreco);

  Fun4AllOutputManager *out = new Fun4AllDstOutputManager("TriggerOut", dstOutput.c_str());
  se->registerOutputManager(out);
  if (nEvents < 0)
  {
    return dstOutput;
  }
  se->run(nEvents);
  se->End();
  delete se;

  return dstOutput;
}

string Fun4All_silicon_data(const int nEvents = 3e3, const int runNumber = 68626, const string outputDir = "dummyDir", Fun4AllRunNodeInputManager* ingeo = nullptr)
{
  string gl1input = outputDir + "/lists/gl1daq.list";
  vector<string> mvtxinput, inttinput;

  for (unsigned int i = 0; i < 6; ++i) mvtxinput.push_back(outputDir + "/lists/mvtx_flx" + to_string(i) + ".list");
  for (unsigned int i = 0; i < 8; ++i) inttinput.push_back(outputDir + "/lists/intt_flx" + to_string(i) + ".list");

  string dstOutput = outputDir + "/files/DST_SILIVTX.root"; 

  Enable::CDB = true;

  Fun4AllServer *se = Fun4AllServer::instance();
  se->Verbosity(1);

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

  i = 0;
  for (auto& file : inttinput)
  {
    auto* sngl= new SingleInttPoolInput("INTT_FLX" + to_string(i));
    sngl->Verbosity(0);
    sngl->SetBcoRange(10);
    sngl->SetNegativeBco(10);
    sngl->streamingMode(true);
    sngl->AddListFile(file);
    inStream->registerStreamingInput(sngl, InputManagerType::INTT);
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
  Intt_HitUnpacking();

  Mvtx_Clustering();
  Intt_Clustering();

  silicon_detector_analyser cluster_analyser = new silicon_detector_analyser();
  se->registerSubsystem(cluster_analyser);

  auto silicon_Seeding = new PHActsSiliconSeeding;
  silicon_Seeding->Verbosity(0);
  silicon_Seeding->setStrobeRange(-1, 1);
  // these get us to about 83% INTT > 1
  silicon_Seeding->setinttRPhiSearchWindow(0.4);
  silicon_Seeding->setinttZSearchWindow(2.0);
  silicon_Seeding->seedAnalysis(false);
  se->registerSubsystem(silicon_Seeding);

  auto merger = new PHSiliconSeedMerger;
  merger->Verbosity(0);
  se->registerSubsystem(merger);

  auto converter = new TrackSeedTrackMapConverter;
  converter->setTrackSeedName("SiliconTrackSeedContainer");
  converter->setFieldMap(G4MAGNET::magfield_tracking);
  converter->Verbosity(0);
  se->registerSubsystem(converter);

  auto finder = new PHSimpleVertexFinder;
  finder->Verbosity(0);
  finder->setDcaCut(2);
  finder->setTrackPtCut(0.1);
  finder->setBeamLineCut(1);
  finder->setTrackQualityCut(1000000000);
  finder->setNmvtxRequired(2);
  finder->setOutlierPairCut(0.5);
  se->registerSubsystem(finder);

  Fun4AllOutputManager *out = new Fun4AllDstOutputManager("SiliconOut", dstOutput.c_str());
  /*
  out->StripNode("MVTXRAWEVTHEADER");
  out->StripNode("MVTXRAWHIT");
  out->StripNode("MVTXEVENTHEADER");
  out->StripNode("INTTRAWHIT");
  out->StripNode("TRKR_MVTXHITSETHELPER");
  out->StripNode("TRKR_HITSET");
  out->StripNode("TRKR_CLUSTER");
  out->StripNode("TRKR_CLUSTERHITASSOC");
  out->StripNode("TRKR_CLUSTERCROSSINGASSOC");
  out->StripNode("SiliconTrackSeedContainer");
  out->StripNode("SvtxTrackMap");
  */
  se->registerOutputManager(out);

  se->run(nEvents);
  se->End();
  delete se;

  return dstOutput;
}

void Fun4All_combineAndAnalyse(const int nEvents = 3e3, const int runNumber = 68626, const string outputDir = "dummyDir", const string mbdInFile = "dummyMBD.root", const string siliconInFile = "dummySilicon.root")
{
  string outputFile = outputDir + "/files/outputVTX.root";

  Fun4AllServer *se = Fun4AllServer::instance();
  se->Verbosity(1);

  Fun4AllDstInputManager *mbdIn = new Fun4AllDstInputManager("MBDInputManager");
  mbdIn->fileopen(mbdInFile.c_str());
  se->registerInputManager(mbdIn);

  auto hitsintrack = new Fun4AllDstInputManager("TrackInputManager");
  hitsintrack->AddFile(siliconInFile.c_str());
  se->registerInputManager(hitsintrack);

  GlobalVertexReco *gblvertex = new GlobalVertexReco();
  gblvertex->Verbosity(0);
  se->registerSubsystem(gblvertex);

  auto compare = new VertexCompare();
  compare->setOutputName(outputFile.c_str());
  se->registerSubsystem(compare);

  se->run(nEvents);
  se->End();
  delete se;
}

void Fun4All_VertexCompare(string runNumber = "68626", string outputDir = "", const int nEvents = 3e3, bool doMBD = true, bool doSilicon = true, bool doCombine = true)
{
  if (outputDir.length() == 0)
  {
    outputDir = "./VertexCompare_run_" + runNumber;
    string subfolders[] = {"files", "lists", "plots"};
    for (auto& subfolder : subfolders)
    {
      string makeDirectory = "mkdir -p " + outputDir + "/" + subfolder;
      system(makeDirectory.c_str());
    }
  }

  int i_runNumber = stoi(runNumber);
  string mbdOutput = outputDir + "/files/DST_MBD.root";
  string siliconOutput = outputDir + "/files/DST_SILIVTX.root";

  Enable::CDB = true;
  auto rc = recoConsts::instance();
  rc->set_IntFlag("RUNNUMBER", i_runNumber);
  rc->set_StringFlag("CDB_GLOBALTAG", "ProdA_2024");
  rc->set_uint64Flag("TIMESTAMP", i_runNumber);

  std::string geofile = CDBInterface::instance()->getUrl("Tracking_Geometry");
  Fun4AllRunNodeInputManager *ingeo = new Fun4AllRunNodeInputManager("GeoIn");
  ingeo->AddFile(geofile);

  if (doMBD) mbdOutput = Fun4All_MBD_data(nEvents, i_runNumber, outputDir);
  if (doSilicon) siliconOutput = Fun4All_silicon_data(nEvents, i_runNumber, outputDir, ingeo);
  if (doCombine) Fun4All_combineAndAnalyse(nEvents, i_runNumber, outputDir, mbdOutput, siliconOutput);

  gSystem->Exit(0);
}

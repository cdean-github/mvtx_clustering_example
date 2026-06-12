// Tell emacs that this is a C++ source
//  -*- C++ -*-.
#ifndef VERTEXCOMPARE_H
#define VERTEXCOMPARE_H

#include <fun4all/SubsysReco.h>

#include <TFile.h>
#include <TTree.h>

#include <string>

class PHCompositeNode;

class VertexCompare : public SubsysReco
{
 public:

  VertexCompare(const std::string &name = "VertexCompare");

  ~VertexCompare() override;

  /** Called during initialization.
      Typically this is where you can book histograms, and e.g.
      register them to Fun4AllServer (so they can be output to file
      using Fun4AllServer::dumpHistos() method).
   */
  int Init(PHCompositeNode *topNode) override;

  /** Called for first event when run number is known.
      Typically this is where you may want to fetch data from
      database, because you know the run number. A place
      to book histograms which have to know the run number.
   */
  int InitRun(PHCompositeNode *topNode) override;

  /** Called for each event.
      This is where you do the real work.
   */
  int process_event(PHCompositeNode *topNode) override;

  /// Clean up internals after each event.
  int ResetEvent(PHCompositeNode *topNode) override;

  /// Called at the end of each run.
  int EndRun(const int runnumber) override;

  /// Called at the end of all processing.
  int End(PHCompositeNode *topNode) override;

  /// Reset
  int Reset(PHCompositeNode * /*topNode*/) override;

  void Print(const std::string &what = "ALL") const override;

  void setOutputName(std::string name) { outFileName = name; };

 private:

  TFile* outFile = nullptr;
  TTree* outTree = nullptr;
  std::string outFileName = "outputVTX.root";

  float mbdVertex {std::numeric_limits<float>::quiet_NaN()};
  float trackerVertex {std::numeric_limits<float>::quiet_NaN()};
  UInt_t nTracks {std::numeric_limits<unsigned int>::quiet_NaN()};
  UInt_t n_MBDVertex {std::numeric_limits<unsigned int>::quiet_NaN()};
  UInt_t n_TRKVertex {std::numeric_limits<unsigned int>::quiet_NaN()};

  bool hasMBD {false};
  bool hasTRK {false};

  int counter = 0;
};

#endif // VERTEXCOMPARE_H

#!/bin/bash

currentDir=$(pwd)
outDir=${currentDir}/Silicon_MBD_Comparisons

nEvents=100

source /cvmfs/sphenix.opensciencegrid.org/alma9.2-gcc-14.2.0/opt/sphenix/core/bin/sphenix_setup.sh -n new

export MYINSTALL=$currentDir/module/install
export LD_LIBRARY_PATH=$MYINSTALL/lib:$LD_LIBRARY_PATH
export ROOT_INCLUDE_PATH=$MYINSTALL/include:$ROOT_INCLUDE_PATH

source /cvmfs/sphenix.opensciencegrid.org/alma9.2-gcc-14.2.0/opt/sphenix/core/bin/setup_local.sh $MYINSTALL

runNumber=80279
fullRunNumber=$(printf "%08d" ${runNumber})
dataTopDir=$outDir/VertexCompare_run_${runNumber}

declare -a folders=("files" "lists" "plots")

for folder in "${folders[@]}"; do
  mkdir -p ${dataTopDir}/${folder}
done

dataDir="/data/sphenix-data/rawData" #MIT
#dataDir="/sphenix/lustre01/sphnxpro/physics" #SCDF

declare -A pairs=(["gl1daq"]="${dataDir}/GL1/physics/GL1_physics_gl1daq-"
                  ["mvtx_flx"]="${dataDir}/MVTX/physics/physics_mvtx")

rawTrailer="-0000.evt"
mbdTrailer="-0000.prdf"
listTrailer=".list"

for i in "${!pairs[@]}"; do
  j=${pairs[$i]}

  if [ "$i" == "gl1daq" ] || [ "$i" == "seb18" ]; then
    file=${j}${fullRunNumber}${rawTrailer}
   
    if [ "$i" == "seb18" ]; then file=${j}${fullRunNumber}${mbdTrailer}; fi;

    if [ -f ${file} ]; then
      ls -1 ${file} > $dataTopDir/${folders[1]}/${i}${listTrailer}
    else
      echo "Missing file"
      exit 1
    fi
  fi

  if [ "$i" == "mvtx_flx" ] || [ "$i" = "intt_flx" ]; then
    nLoops=5
    if  [ "$i" == "intt_flx" ]; then nLoops=7; fi;

    for k in $(seq 0 $nLoops); do
      file=${j}${k}-${fullRunNumber}${rawTrailer}
      if [ -f ${file} ]; then
        ls -1 ${file} > $dataTopDir/${folders[1]}/${i}${k}${listTrailer}
      else
        echo "Missing file"
        exit 1
      fi
    done
  fi

done

echo "Running macros"
root -l -q -b Fun4All_mvtxClustering.C\(${runNumber},\"${dataTopDir}\",${nEvents}\)

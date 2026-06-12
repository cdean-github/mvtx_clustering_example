source /cvmfs/sphenix.opensciencegrid.org/alma9.2-gcc-14.2.0/opt/sphenix/core/bin/sphenix_setup.sh -n new
cd module
mkdir install
mkdir build
cd build
../autogen.sh --prefix=$(pwd)/../install
make -j $(nproc) && make install
cd ../../

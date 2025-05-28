set -e

cd docker
rm -rf build; mkdir build; cd build
# cd build

# clone the repositories
git clone --depth 1 git@github.com:grammaright/prevision.git
git clone --depth 1 --recurse-submodules git@github.com:grammaright/duckdb-spatial.git
git clone --depth 1 --recurse-submodules --branch v1.0.0 git@github.com:duckdb/duckdb.git 
git clone --depth 1 --recurse-submodules --branch m2bench-improved git@github.com:grammaright/m2bench.git 

cd duckdb-spatial/duckdb; git checkout 1f98600c2c; git fetch --unshallow || true; cd ../..
cd duckdb; git checkout 1f98600c2c; cd ..

# copy CMakeUserPresets.json
cp ../CMakeUserPresets.json prevision
cp ../CMakeUserPresets.json m2bench/Impl/polyglot/run_tasks

# build the docker images
sudo docker build -t grammaright/polyglot-improved:latest ../..

cd ../..

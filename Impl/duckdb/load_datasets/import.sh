#DATA_PATH=$(pwd)/../../../Datasets/
DATA_PATH=/media/nvme2/Workspace/mxm/tileduck/m2bench-dataset/sf1
#DATA_PATH=/media/nvme2/Workspace/mxm/tileduck/m2bench-dataset/sf2
#DATA_PATH=/media/nvme2/Workspace/mxm/tileduck/m2bench-dataset/sf5
#DATA_PATH=/media/nvme2/Workspace/mxm/tileduck/m2bench-dataset/sf10


rm -rf /tmp/m2bench/
mkdir -p /tmp/m2bench/
ln -s $DATA_PATH/ecommerce /tmp/m2bench/ 2>&1 >& /dev/null
ln -s $DATA_PATH/healthcare /tmp/m2bench/ 2>&1 >& /dev/null 
ln -s $DATA_PATH/disaster /tmp/m2bench/ 2>&1 >& /dev/null


echo "==== Disaster & Safety ===="
cd disaster
bash load_disaster.sh
cd ..

sudo rm -rf ./docker/database/tilestore/__*.tilestore
sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
sudo docker run -it --shm-size=30G -v $(pwd)/docker/database:/data/database -v $(pwd)/docker/eval:/data/eval grammaright/polyglot-improved:latest bash /data/eval/eval.sh $1 $2 $3
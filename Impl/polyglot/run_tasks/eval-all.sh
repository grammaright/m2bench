echo "========================================================================"
echo "Evaluate M2Bench Polyglot"
echo "========================================================================"

NUM_ITER=4

function run() {
    TASK=$1
    SF=$2
    for i in $(seq 1 $NUM_ITER); do
        echo "========================================================================"
        echo "Task $TASK"
        echo "Iteration $i / $NUM_ITER"
        echo "========================================================================"
        bash eval-docker.sh $TASK $SF 0
    done
}

run 0 1
run 1 1
run 2 1
run 9 1
run 14 1
run 15 1
run 16 1

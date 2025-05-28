################################################
# Environment variables for evaluation
################################################

export TILESTORE_BASE_DIR=/data/database/tilestore

export BF_DATA_SIZE=24000000000
export BF_IDATA_SIZE=0
export BF_KEYSTORE_SIZE=1342177280
export BF_BFSTORE_SIZE=1342177280
export BF_EVICTION_POLICY=2
export BF_PREEMPTIVE_EVICTION=0
export BF_LRUK_K=0
export BF_LRUK_CRP=0

export OMP_NUM_THREADS=1
export OPENBLAS_NUM_THREADS=1
export MKL_NUM_THREADS=1
export VECLIB_MAXIMUM_THREAD=1
export NUMEXPR_NUM_THREADS=1
export __PREVISION_NUM_THREADS=1

################################################
# Configure directories and move to database directory
################################################

mkdir -p $TILESTORE_BASE_DIR
cd /data/database

################################################
# Run
################################################
${M2_PATH}/build/m2bench $1 $2 $3
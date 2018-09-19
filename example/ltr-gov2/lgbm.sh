#!/bin/bash

set -ue

SPATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BASE=$SPATH/../..
LGBM=$SPATH/../../../ltr-baseline/bin/lightgbm

mkdir -p model run eval

dat="$SPATH"

trees=2000
leaves=15
eta=0.05
estop=100
name="lgbm.goss.${trees}.${leaves}.${eta}"
for i in {1..5}; do
suffix="${name}.fold${i}"
qrels="${dat}/f${i}/test.qrels"

# training
$LGBM \
    app=lambdarank \
    save_binary=true \
    boosting=goss \
    num_trees=$trees \
    num_leaves=$leaves \
    learning_rate=$eta \
    feature_fraction=1.0 \
    metric=ndcg \
    eval_at=150 \
    early_stopping_round=$estop \
    output_model="model/model.${suffix}" \
    group_column=0 \
    data="${dat}/f${i}/train.csv" \
    valid_data="${dat}/f${i}/val.csv"

# prediction
$LGBM \
    task=predict \
    data="${dat}/f${i}/test.csv" \
    input_model="model/model.${suffix}" \
    output_result="run/score.${suffix}"

# build run file for current fold
paste -d' ' run/score.${suffix} $qrels \
    | awk '{print $2, "Q0", $4, 0, $1, "lgbm"}' \
    | sort -k1n -k5nr \
    | $BASE/script/trecrank.awk > run/run.${suffix}
done

# save complete run file for the 150 topics on Gov2
cat $SPATH/run/run.${name}.fold? > $SPATH/run/run.all.${name}
# evaluate final run file
# $BASE/script/eval.sh $dat/gov2.qrels $SPATH/run/run.all.${name} > $SPATH/eval/eval.all.${name}
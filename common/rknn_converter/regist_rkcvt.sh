# export RKCVT_DIR=$(dirname $0)
# export RKCVT=$RKCVT_DIR/rknn_convert.py
# export RKCVT_INIT=$RKCVT_DIR/config_init.py
# export RKCVT=$(pwd | sed 's/\(rknn_model_zoo\).*/\1/g')/common/rknn_converter/rknn_convert.py
# export RKCVT_INIT=$(pwd | sed 's/\(rknn_model_zoo\).*/\1/g')/common/rknn_converter/config_init.py
# bash

RKCVT_DIR=$(realpath "$0")
RKCVT_DIR=$(dirname "$RKCVT_DIR")
RKCVT_EXAMPLE_CONFIG=$RKCVT_DIR/example_model/shufflenet_config.yml


RKCVT=$RKCVT_DIR/rknn_convert.py
RKCVT_INIT=$RKCVT_DIR/config_init.py
echo "Please execute the following command to set environment variables:"
echo "export RKCVT=$RKCVT"
echo ""
echo "Optional:"
echo "export RKCVT_INIT=$RKCVT_INIT"
echo "Example config: $RKCVT_EXAMPLE_CONFIG"
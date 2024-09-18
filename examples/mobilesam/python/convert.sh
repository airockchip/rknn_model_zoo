# 内部测试用

set -e

# echo "$0 $@"
while getopts ":m:t:d:o:" opt; do
  case $opt in
    m)
      MODEL=$OPTARG
      ;;
    t)
      TARGET_PLATFORM=$OPTARG
      ;;
    d)
      DTYPE=$OPTARG
      ;;
    o)
      OUTPUT=$OPTARG
      ;;
    :)
      echo "Option -$OPTARG requires an argument." 
      exit 1
      ;;
    ?)
      echo "Invalid option: -$OPTARG index:$OPTIND"
      ;;
  esac
done

if [[ ${MODEL} == *"encoder_tiny.onnx"* ]]; then
    python ./encoder/convert.py ${MODEL} ${TARGET_PLATFORM} ${DTYPE} ${OUTPUT}
fi

if [[ ${MODEL} == *"decoder.onnx"* ]]; then
    python ./decoder/convert.py ${MODEL} ${TARGET_PLATFORM} ${DTYPE} ${OUTPUT}
fi



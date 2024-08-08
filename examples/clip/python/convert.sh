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

if [[ ${MODEL} == *"clip_images.onnx"* ]]; then
    python ./images/convert.py ${MODEL} ${TARGET_PLATFORM} ${DTYPE} ${OUTPUT}
fi

if [[ ${MODEL} == *"clip_text.onnx"* ]]; then
    python ./text/convert.py ${MODEL} ${TARGET_PLATFORM} ${DTYPE} ${OUTPUT}
fi



#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

[[ ! "${OPENCV_QUIET}" ]] && ( echo "Setting vars for OpenCV 3.4.5" )
export LD_LIBRARY_PATH="$SCRIPT_DIR/../lib:$LD_LIBRARY_PATH"

if [[ ! "$OPENCV_SKIP_PYTHON" ]]; then
  PYTHONPATH_OPENCV="$SCRIPT_DIR/python_loader_is_not_installed"
  [[ ! "${OPENCV_QUIET}" ]] && ( echo "Append PYTHONPATH: ${PYTHONPATH_OPENCV}" )
  export PYTHONPATH="${PYTHONPATH_OPENCV}:$PYTHONPATH"
fi

# Don't exec in "sourced" mode
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  if [[ $# -ne 0 ]]; then
    [[ ! "${OPENCV_QUIET}" && "${OPENCV_VERBOSE}" ]] && ( echo "Executing: $*" )
    exec "$@"
  fi
fi

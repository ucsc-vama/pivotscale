# Shell script to convert SNAP graphs into the .sg format required by PivotScale
# Usage: bash ConvertSNAP.sh <PATH_TO_GRAPH>/<graph>.txt
# Output: <PATH_TO_GRAPH>/graphU.sg which should be used as input in PivotScale

if [ $# -ne 1 ]; then
  echo "Error: Please pass input file name"
	echo "Usage: ./ConvertSNAP <PATH_TO_GRAPH>/<graph>.txt"

  exit 1
fi

make converter

SNAP_FILE=$1
# Remove comments from SNAP File
sed -i '/#/d' ${SNAP_FILE} 

# Rename graph.txt to graph.el
INPUT_GRAPH="${SNAP_FILE%.txt}.el"
mv ${SNAP_FILE} ${INPUT_GRAPH}

# Convert graph to .sg
OUTPUT_GRAPH="${INPUT_GRAPH%.el}U.sg"
./converter -sf ${INPUT_GRAPH} -b ${OUTPUT_GRAPH}

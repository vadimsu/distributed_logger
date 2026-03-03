#!/bin/bash
project_root=./
header_file_path=
if [ $# -lt 1 ];then
	echo "need at least a header file path"
	exit 1
fi
header_file_path=$1
if [ $# -ge 2 ];then
	project_root=$2
fi
python3 -m cxxheaderparser --mode=json "$header_file_path" > parsed_header.json
./tools/parse_json.py parsed_header.json "$project_root"

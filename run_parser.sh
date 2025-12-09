#!/bin/bash
python3 -m cxxheaderparser --mode=json myheader.hh > parsed_header.json
./parse_json.py parsed_header.json

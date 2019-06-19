#!/bin/bash

# 
# Fetch latest bom's from Macrofab.
#
# J. Adams <jna@retina.net> 6/2019
#

# You can pretty-print these json outputs with the jsonpp utility, available via:
#    brew install jsonpp
#
# set APIKEY in your environment prior to calling this script.
#

# get the bill of materials
curl -H 'Accept: application/json' \
        https://api.macrofab.com/api/v3/pcb/68v44h8/1/bom\?apikey=$APIKEY > bom.json

# and then get the positions for the board. For some reason you can't
# get these in API v3.
curl -H 'Accept: application/json' \
        https://api.macrofab.com/api/v2/pcb/68v44h8/1/xyrs\?apikey=$APIKEY > bom.xyrs


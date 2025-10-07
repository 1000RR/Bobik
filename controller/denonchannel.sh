#!/bin/bash
THISDIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
source $THISDIR/denonaddress.bash
wget -O /dev/null $DENONADDRESS/MainZone/index.put.asp?cmd0=PutZone_InputFunction%2F$1


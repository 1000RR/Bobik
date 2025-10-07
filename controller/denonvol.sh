#!/bin/bash
SCRIPTDIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
source $SCRIPTDIR/denonaddress.bash
VOL=$(($1-81)) #subtract 81 always - db scale conversion
amixer set Master 100% unmute
wget -O /dev/null $DENONADDRESS/MainZone/index.put.asp?cmd0=PutMasterVolumeSet%2F$VOL


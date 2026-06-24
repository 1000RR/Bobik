#!/bin/bash
SCRIPTDIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
source $SCRIPTDIR/denonaddress.bash
curl --connect-timeout .5 -s "http://$DENONADDRESS/goform/formMainZone_MainZoneXml.xml?_=1694976906082" \
    -H "Accept: */*"   -H "Accept-Language: en-US,en"   -H "Cache-Control: no-cache"   -H "Connection: keep-alive" \
    -H "Pragma: no-cache"   -H "Referer: http://$DENONADDRESS/MainZone/index.html"   -H "Sec-GPC: 1" \
    -H "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/117.0.0.0 Safari/537.36" \
    -H "X-Requested-With: XMLHttpRequest"   -H "dnt: 1"   --compressed   --insecure \
    | grep -v "xml" | xmllint --xpath "//item/MasterVolume/value/text()" -
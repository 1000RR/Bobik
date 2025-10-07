#!/bin/bash
SCRIPTDIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
source $SCRIPTDIR/denonaddress.bash
curl --connect-timeout .5 "http://$DENONADDRESS/MainZone/index.put.asp" \
  -H "Accept: */*" \
  -H "Accept-Language: en-US,en" \
  -H "Connection: keep-alive" \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -H "Origin: http://$DENONADDRESS" \
  -H "Referer: http://$DENONADDRESS/MainZone/index.html" \
  -H "Sec-GPC: 1" \
  -H "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36" \
  -H "X-Requested-With: XMLHttpRequest" \
  -H "dnt: 1" \
  --data-raw "cmd0=PutSystem_OnStandby%2FON&cmd1=aspMainZone_WebUpdateStatus%2F&cmd2=PutZone_InputFunction/SAT%2FCBL" \
  --compressed \
  --insecure

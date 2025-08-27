#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DATE=`openssl x509 -in $SCRIPT_DIR/bobik-cert.pem -noout -dates | grep notAfter | awk '{sub(/^notAfter=/, ""); print}'`

OS=$(uname)

if [[ "$OS" == "Linux" ]]; then
   	OUTPUT=`date -d "$DATE" '+%Y-%m-%d %H:%M:%S %z'`
elif [[ "$OS" == "Darwin" ]]; then
    OUTPUT=`date -j -f "%b %d %H:%M:%S %Y %Z" "$DATE" "+%Y-%m-%d %H:%M:%S %z"`
else
    OUTPUT=""
fi

echo $OUTPUT

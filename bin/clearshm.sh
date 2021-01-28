#!/bin/bash
ipcs -m | grep ${USER:0:10} | awk '{if ($NF==0) print $2}' | xargs -i ipcrm -m {}
ipcs -m | grep ${USER:0:10}


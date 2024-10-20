#!/bin/sh
cd $(dirname ${0})
${PWD}/bin/vds --gamepath ${PWD} ${@}

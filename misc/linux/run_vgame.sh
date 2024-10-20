#!/bin/sh
cd $(dirname ${0})
${PWD}/bin/vgame --gamepath ${PWD} ${@}

@echo off
pushd "%~dp0"
bin/vgame.exe --gamepath "%~dp0" %*
popd

@echo off
pushd "%~dp0"
bin/vds.exe --gamepath "%~dp0" %*
popd

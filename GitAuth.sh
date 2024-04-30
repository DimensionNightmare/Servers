#!/bin/bash
cd $(dirname $0)

git remote remove origin

PAT=x5kc3a7vzipj2txyieflgnhpd5re4jtibsduskdcezj2bpvrthsa

git remote add origin https://$PAT@dev.azure.com/DimensionNightmare/Servers/_git/Servers

git fetch

git branch --set-upstream-to=origin/main main
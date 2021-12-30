#!/usr/bin/env python3

import os
import re
from dotenv import load_dotenv


load_dotenv()

fusePath=os.getenv('FUSEPATH')
repoPath="./"

if not fusePath:
  raise Exception("make sure run a 'fetch' first to have the '.env' file created. there set FUSEPATH accordingly.")

with open(repoPath+".clobber/Incubator.fuse", "r") as f:
  incubatorFuse=f.read()

incubatorFuse=re.sub(r'\s+local\ss+REPOSITORYPATH\s*=\s*\[\[[^\]]*\]\]', '\n\n  local REPOSITORYPATH=[['+repoPath+']]',incubatorFuse)

with open(fusePath+"Incubator_temp.fuse", "w") as f:
  f.write(incubatorFuse)

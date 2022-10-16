#!/usr/bin/env python3

import os
import re
from dotenv import load_dotenv


load_dotenv()

fusePath=os.getenv('FUSEPATH')
repoPath=os.getenv('REPOPATH')

if True or not fusePath or not repoPath:
  print("""
    make sure run a 'fetch' first to have the '.env' file created.
    then set FUSEPATH and REPOPATH accordingly.
    make sure to have the paths end with trailing slashes.
    """)
else:

  with open(repoPath+".clobber/Incubator.fuse", "r") as f:
    incubatorFuse=f.read()

  incubatorFuse=re.sub(r'\s+local\s+REPOSITORYPATH\s*=\s*\[\[[^\]]*\]\]', '\n\n  local REPOSITORYPATH=[['+repoPath+']]',incubatorFuse)

  with open(fusePath+"Incubator_temp.fuse", "w") as f:
    f.write(incubatorFuse)
    print(f"copied '.clobber/Incubator.fuse' from '{repoPath}' as 'Incubator_temp.fuse' to '{fusePath}'")

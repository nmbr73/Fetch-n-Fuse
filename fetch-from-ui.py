#!/usr/bin/env python3

import fetch
import os
from dotenv import load_dotenv

print("\n# ----- FETCH VIA UI")

selfpath = os.path.dirname(sys.argv[0])+os.sep

print("# SELFPATH:         ",selfpath)
print("# ARGV:             ",sys.argv, len(sys.argv))
print("# Folder:           ",folder)
print("# ID:               ",id)


if folder != "":
    folder = folder + os.sep

fetch.CONVERSIONS_PATH = selfpath + "Conversions" + os.sep + folder
fetch.ASSETS_PATH      = selfpath + "Assets"+os.sep
fetch.NOASSETS         = True
fetch.VERBOSE          = False 

load_dotenv(selfpath+".env")


print("# CONVERSIONS_PATH: ",fetch.CONVERSIONS_PATH)
print("# ASSETS_PATH:      ",fetch.ASSETS_PATH)
print("# AUTHOR:           ",os.getenv('AUTHOR'))
print("# DOWNLOADS:        ",os.getenv('DOWNLOADS'))
print("# FUSEPATH:         ",os.getenv('FUSEPATH'))
print("# REPOPATH:         ",os.getenv('REPOPATH'))

fetch.do_fetch(id)

#!/usr/bin/python

import csv
import sys

if len(sys.argv) != 2: 
  print "Usage: %s filename\n", sys.argv[0]
  sys.exit(1)

with open("mfbom.tsv","wb") as tsvout, open(sys.argv[1],"rb") as fin:
    tsvout = csv.writer(tsvout, delimiter='\t')

    # toss the header
    line = fin.readline()
#    tsvout.writerow(["DESIGNATOR", "MPN", "DESCRIPTION", "STOCK"])
    tsvout.writerow(["DESIGNATOR", "MPN", "DESCRIPTION"])
    while True:
        line = fin.readline() # designator

        if not line:
            break
        line2 = fin.readline() # mpn
        line3 = fin.readline() # DESCRIPTION
        line4 = fin.readline() # blank
        line5 = fin.readline() # STOCK
        tsvout.writerow([ line.lstrip().rstrip(), line2.lstrip().rstrip(), line3.lstrip().rstrip() ] )
#        tsvout.writerow([ line.lstrip().rstrip(), line2.lstrip().rstrip(), line3.lstrip().rstrip(), line5.lstrip().rstrip() ])

#!/usr/local/bin/python

import csv

with open("mfbom.tsv","wb") as tsvout, open("mf_bom_scrape.txt","rb") as fin:
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

#!/usr/local/bin/python

import csv

PL = {}
OLDBOM = []

# load both bom files

def appendpl(fn):
    
  bf = open(fn)

  for line in bf:
      if line[0] != "#": 
          x = line.split()
          print x
          PL[x[0]] = x

appendpl('DC27-badge-top.pos')
appendpl('DC27-badge-bottom.pos')          

# now read through the exported file
with open('bom.tsv','rb') as tsvin, open('new.tsv', 'wb') as tsvout:
    tsvin = csv.reader(tsvin, delimiter='\t')
    tsvout = csv.writer(tsout, delimiter='\t')
    
    for row in tsvin:
        OLDBOM.append(row)
        for d in row[0].split(","):
            if d != "":
                print d.lstrip().rstrip()
                # designator, value, package, mpn, xorgin, yorigin, width, height

                tsvout.writerow(

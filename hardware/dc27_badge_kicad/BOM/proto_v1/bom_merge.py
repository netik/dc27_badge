#!/usr/local/bin/python

import csv

PL = {}
OLDBOM = []

# load both bom files

def appendpl(fn):
  with open(fn, "rb") as csvin:
    csvin = csv.reader(csvin)

    for row in csvin:
      print row
      PL[row[0]] = row

#appendpl('./DC27-badge-top.pos')
#appendpl('./DC27-badge-bottom.pos')
appendpl('../gerbers/DC27-badge-all-pos.csv')

# now read through the exported file
with open('bom.tsv','rb') as tsvin, open('new.tsv', 'wb') as tsvout:
    tsvin = csv.reader(tsvin, delimiter='\t')
    tsvout = csv.writer(tsvout, delimiter='\t')

    # using the placement files as a reference, plug in the data from the
    # bom.
    linenum = 0
    for row in tsvin:
        if linenum == 0:
            tsvout.writerow(["DESIGNATOR", "VALUE", "PACKAGE", "MPN", "XORIGIN", "YORIGIN", "WIDTH", " HEIGHT"])
            linenum = linenum + 1
            continue
        linenum = linenum + 1

        OLDBOM.append(row)
        for d in row[0].split(","):  # row is the row in the tsv.
            if d != "":
                des = d.lstrip().rstrip()
                # PL is the placement file
                # designator, value, package, mpn, xorgin, yorigin, width, height
                if PL.has_key(des):
                  tsvout.writerow([des, row[1], row[6], row[5], PL[des][3], PL[des][4], 0, 0 ])
                else:
                  print "warning: %s not found in placement file" % des

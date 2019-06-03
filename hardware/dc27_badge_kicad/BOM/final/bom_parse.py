#!/usr/bin/python

f = open("BOM-FINAL-SCRAPE.TXT")

i = 0
des = ""
mpn = ""

for line in f:
    if i == 0:
        des = line

    if i == 1:
        mpn = line

        print "%s\t%s" % ( des.lstrip().rstrip(), mpn.lstrip().rstrip())

    i = i + 1

    if i == 5: 
        i = 0



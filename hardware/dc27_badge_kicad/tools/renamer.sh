#!/bin/bash
# macrofab rules

# .GTL Top Layer
# .GTO Top Silkscreen
# .GTS Top Soldermask
# .GTP Top SMD Paste
# .GBL Bottom Layer
# .GBO Bottom Silkscreen
# .GBS Bottom Soldermask
# .GBP Bottom SMD Paste
# .BOR Board Outline (Edge Cuts) *Place routed holes on this layer*
# .G2L ONLY for four layer board
# .G3L ONLY for four layer board
# .XLN NC Drill File

# drill file
mv DC27-badge.drl DC27-badge.xln

# edge
mv DC27-badge-Edge_Cuts.gm1 DC27-badge-Edge_Cuts.bor





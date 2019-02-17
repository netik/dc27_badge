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

# layer 2/3
mv DC27-badge-GND.g2 DC27-badge-GND.g2l
mv DC27-badge-PWR.g3 DC27-badge-PWR.g3l

# edge
mv DC27-badge-Edge.Cuts.gm1 DC27-badge-Edge.Cuts.bor






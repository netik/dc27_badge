Ship filename readme:

Ships are identified by the following filename structure:

s2e-h-l.tif
 ^^ ^ ^
 || | +---- Orientation (L)eft facing or (R)ight facing
 || +------ Frame Type (see below)
 |+-------- 'e' enemy or 'p' player. Basically, Red, or Black. 
 +--------- Ship class number (see ships.c/ships.h)

Ship class numbers

0:   PT_Boat
1:   Patrol_Ship
2:   Destroyer
3:   Cruiser
4:   Frigate
5:   Battleship
6:   Submarine
7:   Tesla_Ship

Frame type
This indicates which animation frame this image is for.

n    normal
h    ship was hit
d    ship was destroyed
g    ship is regenerating
t    ship is teleporting
s    ship has shields up
u    (u-boat) ship is submerged



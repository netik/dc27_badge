import random

suits = 'CDHS'
cards = 'A23456789TJQK'
deck = [ j + i for i in suits for j in cards ] + [ 'FJ', 'SJ' ]

r = random.Random()
r.seed(230987459872341398732)
shuffled = ''.join(r.sample(deck, len(deck)))
nperrow = 16
for i in xrange(0, len(shuffled), nperrow):
	print shuffled[i:i + nperrow]

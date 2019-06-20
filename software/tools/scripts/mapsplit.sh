#convert seabattle-4x4-map.tif -crop 320x240 map-%02d.tif
#convert seabattle-4x4-map.tif -resize 320x240 world.tif

convert eganworld.tif -crop 320x240 map-%02d.tif
convert eganworld.tif -resize 320x240 world.tif

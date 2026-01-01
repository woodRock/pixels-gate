from PIL import Image
import numpy as np

img = Image.open("assets/isometric tileset/separated images/tile_000.png")
data = np.array(img)

for y in range(32):
    row = data[y]
    non_transparent = np.where(row[:, 3] > 0)[0]
    if len(non_transparent) > 0:
        print(f"Row {y:2}: {len(non_transparent):2} pixels from {min(non_transparent):2} to {max(non_transparent):2}")
    else:
        print(f"Row {y:2}: Empty")

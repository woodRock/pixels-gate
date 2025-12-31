import struct

def write_bmp(filename, width, height, pixels):
    # BMP Header
    file_size = 14 + 40 + (width * height * 4)
    offset = 54
    header = b'BM' + struct.pack('<I', file_size) + b'\x00\x00\x00\x00' + struct.pack('<I', offset)
    
    # DIB Header (BITMAPINFOHEADER)
    dib_header = struct.pack('<I', 40) + \
                 struct.pack('<i', width) + \
                 struct.pack('<i', height) + \
                 struct.pack('<H', 1) + \
                 struct.pack('<H', 32) + \
                 b'\x00\x00\x00\x00' * 6 # Compression, ImageSize, Xppm, Yppm, ClrUsed, ClrImportant

    with open(filename, 'wb') as f:
        f.write(header)
        f.write(dib_header)
        # Pixels (BGRA, bottom-up)
        for y in range(height - 1, -1, -1):
            for x in range(width):
                r, g, b, a = pixels[y][x]
                f.write(struct.pack('BBBB', b, g, r, a))

def create_key():
    w, h = 32, 32
    pixels = [[(0, 0, 0, 0) for _ in range(w)] for _ in range(h)]
    
    # Gold color
    gold = (255, 215, 0, 255)
    dark_gold = (184, 134, 11, 255)
    
    # Loop (Head)
    for y in range(20, 28):
        for x in range(12, 20):
            if (x-15.5)**2 + (y-23.5)**2 <= 16:
                pixels[y][x] = gold
            if 9 < (x-15.5)**2 + (y-23.5)**2 <= 16:
                pixels[y][x] = dark_gold
    # Hole in loop
    for y in range(22, 26):
        for x in range(14, 18):
            pixels[y][x] = (0,0,0,0)

    # Shaft
    for y in range(6, 20):
        pixels[y][15] = gold
        pixels[y][16] = gold
    
    # Teeth
    pixels[8][17] = gold
    pixels[8][18] = gold
    pixels[10][17] = gold
    pixels[10][18] = gold
    
    write_bmp('assets/key.png', w, h, pixels) # Saving as .png but it's actually BMP format, SDL might detect signature or fail if extension mismatches content type. Safer to save as .bmp
    write_bmp('assets/key.bmp', w, h, pixels)

def create_tools():
    w, h = 32, 32
    pixels = [[(0, 0, 0, 0) for _ in range(w)] for _ in range(h)]
    
    metal = (192, 192, 192, 255)
    dark_metal = (105, 105, 105, 255)
    
    # Pick 1 (Hook)
    for i in range(12):
        pixels[10+i][10+i] = metal
    pixels[22][22] = metal
    pixels[22][21] = metal
    pixels[23][21] = metal
    
    # Pick 2 (Tension Wrench)
    for i in range(12):
        pixels[10+i][20-i] = dark_metal
    pixels[22][10] = dark_metal
    pixels[22][11] = dark_metal
    
    write_bmp('assets/thieves_tools.png', w, h, pixels)
    write_bmp('assets/thieves_tools.bmp', w, h, pixels)

if __name__ == '__main__':
    create_key()
    create_tools()

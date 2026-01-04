import os
from PIL import Image, ImageDraw

def create_icon(filename, draw_func, color=(200, 200, 200)):
    size = (32, 32)
    img = Image.new('RGBA', size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw_func(draw, size, color)
    img.save(filename)
    print(f"Generated {filename}")

def ensure_dir(path):
    if not os.path.exists(path):
        os.makedirs(path)

def draw_wolfpelt(d, s, c):
    # Grey furry shape
    d.polygon([(8, 4), (24, 4), (28, 12), (24, 28), (8, 28), (4, 12)], fill=(120, 120, 120))
    d.line([(8, 4), (24, 4)], fill=(150, 150, 150), width=2) # Top fur

def draw_letter(d, s, c):
    # Paper
    d.rectangle([6, 4, 26, 28], fill=(240, 240, 220), outline=(200, 200, 180))
    # Lines
    d.line([8, 8, 24, 8], fill=(50, 50, 50), width=1)
    d.line([8, 12, 24, 12], fill=(50, 50, 50), width=1)
    d.line([8, 16, 24, 16], fill=(50, 50, 50), width=1)
    # Seal
    d.ellipse([14, 20, 18, 24], fill=(200, 0, 0))

def draw_magicring(d, s, c):
    # Ring
    d.ellipse([6, 6, 26, 26], outline=(255, 215, 0), width=3)
    # Gem
    d.ellipse([12, 4, 20, 12], fill=(0, 255, 255))

def draw_stagmeat(d, s, c):
    # Similar to boar but maybe different shape
    d.ellipse([4, 8, 28, 24], fill=(160, 60, 60))
    d.rectangle([12, 12, 20, 20], fill=(200, 100, 100)) # Marbling

def draw_badgerpelt(d, s, c):
    # Stripey pelt
    d.polygon([(6, 6), (26, 6), (22, 26), (10, 26)], fill=(50, 50, 50))
    d.rectangle([14, 6, 18, 26], fill=(200, 200, 200)) # Stripe

def main():
    ensure_dir("assets/ui")
    create_icon("assets/ui/item_wolfpelt.png", draw_wolfpelt)
    create_icon("assets/ui/item_letter.png", draw_letter)
    create_icon("assets/ui/item_magicring.png", draw_magicring)
    create_icon("assets/ui/item_stagmeat.png", draw_stagmeat)
    create_icon("assets/ui/item_badgerpelt.png", draw_badgerpelt)

if __name__ == "__main__":
    main()
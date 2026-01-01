
import os
from PIL import Image, ImageDraw

def create_icon(filename, draw_func, color=(200, 200, 200)):
    size = (32, 32)
    img = Image.new('RGBA', size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Draw background (optional, or transparent)
    # draw.rectangle([0, 0, 31, 31], fill=(50, 50, 50, 255), outline=(100, 100, 100, 255))
    
    draw_func(draw, size, color)
    
    img.save(filename)
    print(f"Generated {filename}")

def ensure_dir(path):
    if not os.path.exists(path):
        os.makedirs(path)

# Drawing functions for each icon
def draw_attack(d, s, c):
    # Sword
    d.line([4, 28, 28, 4], fill=c, width=3)
    d.line([24, 8, 28, 4], fill=c, width=3) # Tip
    d.line([8, 24, 4, 28], fill=c, width=3) # Hilt bottom
    d.line([6, 22, 10, 26], fill=(150, 100, 50), width=5) # Crossguard

def draw_jump(d, s, c):
    # Up Arrow / Boot
    d.polygon([(16, 4), (8, 16), (24, 16)], fill=c)
    d.rectangle([12, 16, 20, 28], fill=c)

def draw_sneak(d, s, c):
    # Eye
    d.ellipse([4, 8, 28, 24], outline=c, width=2)
    d.ellipse([12, 12, 20, 20], fill=c)

def draw_shove(d, s, c):
    # Hand / Push
    d.rectangle([8, 8, 24, 24], fill=c)
    d.line([4, 16, 8, 16], fill=c, width=2) # Force lines
    d.line([4, 12, 8, 12], fill=c, width=2)
    d.line([4, 20, 8, 20], fill=c, width=2)

def draw_dash(d, s, c):
    # Running Man / Lines
    d.line([4, 16, 20, 16], fill=c, width=3)
    d.polygon([(20, 12), (20, 20), (28, 16)], fill=c) # Arrow
    d.line([8, 22, 24, 22], fill=(150, 150, 150), width=2) # Wind

def draw_endturn(d, s, c):
    # Hourglass / Stop
    d.polygon([(8, 4), (24, 4), (16, 16)], fill=c) # Top
    d.polygon([(8, 28), (24, 28), (16, 16)], fill=c) # Bottom

def draw_fireball(d, s, c):
    # Fire ball
    d.ellipse([8, 8, 24, 24], fill=(255, 100, 0))
    d.ellipse([12, 12, 20, 20], fill=(255, 200, 0))

def draw_heal(d, s, c):
    # Cross / Heart
    d.rectangle([14, 6, 18, 26], fill=(0, 255, 0))
    d.rectangle([6, 14, 26, 18], fill=(0, 255, 0))

def draw_magicmissile(d, s, c):
    # Missiles
    d.polygon([(4, 28), (12, 20), (8, 28)], fill=(200, 0, 255))
    d.polygon([(14, 18), (22, 10), (18, 18)], fill=(200, 0, 255))
    d.polygon([(20, 12), (28, 4), (24, 12)], fill=(200, 0, 255))

def draw_shield(d, s, c):
    # Shield
    d.polygon([(6, 6), (26, 6), (26, 20), (16, 28), (6, 20)], fill=(0, 150, 255), outline=c)

def draw_potion(d, s, c):
    # Bottle
    d.ellipse([10, 12, 22, 24], fill=(255, 50, 50)) # Liquid
    d.rectangle([14, 6, 18, 12], fill=(200, 200, 200)) # Neck
    d.line([12, 12, 20, 12], fill=(200, 200, 200), width=1)

def draw_bread(d, s, c):
    # Bread
    d.ellipse([6, 10, 26, 22], fill=(200, 150, 50))
    d.line([10, 12, 10, 20], fill=(150, 100, 30), width=1)
    d.line([16, 12, 16, 20], fill=(150, 100, 30), width=1)
    d.line([22, 12, 22, 20], fill=(150, 100, 30), width=1)

def draw_raregem(d, s, c):
    # Gem
    d.polygon([(16, 4), (28, 12), (16, 28), (4, 12)], fill=(0, 255, 255))
    d.polygon([(16, 8), (24, 14), (16, 24), (8, 14)], fill=(200, 255, 255))

def draw_coins(d, s, c):
    # Coins
    d.ellipse([4, 16, 16, 28], fill=(255, 215, 0), outline=(150, 100, 0))
    d.ellipse([12, 12, 24, 24], fill=(255, 215, 0), outline=(150, 100, 0))
    d.ellipse([8, 4, 20, 16], fill=(255, 215, 0), outline=(150, 100, 0))

def draw_boarmeat(d, s, c):
    # Meat
    d.ellipse([6, 10, 26, 24], fill=(150, 50, 50))
    d.rectangle([20, 12, 28, 16], fill=(255, 255, 255)) # Bone

def draw_rest(d, s, c):
    # Campfire
    # Logs
    d.rectangle([8, 22, 24, 26], fill=(100, 50, 0))
    d.rectangle([10, 24, 22, 28], fill=(120, 60, 0))
    # Flames
    d.polygon([(16, 4), (10, 22), (22, 22)], fill=(255, 100, 0))
    d.polygon([(16, 8), (12, 22), (20, 22)], fill=(255, 200, 0))

def main():
    ensure_dir("assets/ui")
    
    # Actions
    create_icon("assets/ui/action_attack.png", draw_attack)
    create_icon("assets/ui/action_jump.png", draw_jump)
    create_icon("assets/ui/action_sneak.png", draw_sneak)
    create_icon("assets/ui/action_shove.png", draw_shove)
    create_icon("assets/ui/action_dash.png", draw_dash)
    create_icon("assets/ui/action_endturn.png", draw_endturn)
    create_icon("assets/ui/action_rest.png", draw_rest)
    
    # Spells
    create_icon("assets/ui/spell_fireball.png", draw_fireball)
    create_icon("assets/ui/spell_heal.png", draw_heal)
    create_icon("assets/ui/spell_magicmissile.png", draw_magicmissile)
    create_icon("assets/ui/spell_shield.png", draw_shield)
    
    # Items
    create_icon("assets/ui/item_potion.png", draw_potion)
    create_icon("assets/ui/item_bread.png", draw_bread)
    create_icon("assets/ui/item_raregem.png", draw_raregem)
    create_icon("assets/ui/item_coins.png", draw_coins)
    create_icon("assets/ui/item_boarmeat.png", draw_boarmeat)

if __name__ == "__main__":
    main()

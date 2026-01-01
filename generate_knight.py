from PIL import Image, ImageDraw

def create_knight():
    width, height = 32, 32
    img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Colors
    outline = (20, 20, 20, 255)
    armor_light = (200, 200, 200, 255)
    armor_shadow = (150, 150, 150, 255)
    helmet_color = (180, 180, 180, 255)
    red = (220, 50, 50, 255) # Plume
    gold = (255, 215, 0, 255) # Hilt
    blade = (230, 240, 255, 255) # Sword blade
    
    # Helper to draw a pixel rect
    def rect(x, y, w, h, color):
        draw.rectangle([x, y, x+w-1, y+h-1], fill=color)

    # -- Isometric-ish Knight Facing SE --
    
    # Left Leg (Back)
    rect(13, 22, 3, 5, armor_shadow)
    
    # Right Leg (Front)
    rect(17, 24, 3, 5, armor_light)

    # Body (Torso)
    # Main block
    rect(12, 14, 9, 9, armor_light)
    # Shadow/Side
    rect(12, 14, 2, 9, armor_shadow)
    
    # Chest detail (Cross?)
    rect(15, 16, 3, 5, red)

    # Head (Helmet)
    rect(13, 6, 7, 7, helmet_color)
    # Visor
    rect(14, 8, 5, 2, (50, 50, 50, 255))
    
    # Plume
    rect(14, 3, 2, 3, red)
    rect(13, 4, 1, 2, red)
    rect(16, 4, 1, 1, red)

    # Right Arm (Holding Sword)
    rect(20, 15, 3, 6, armor_light)
    
    # Sword
    # Hilt
    rect(21, 20, 2, 2, gold)
    # Guard
    rect(19, 19, 6, 1, gold)
    # Blade (pointing up)
    rect(21, 8, 2, 11, blade)

    # Left Arm (Shield side?)
    rect(9, 16, 3, 5, armor_shadow)
    
    # Shield (Simple Round/Heater)
    # Let's make a heater shield
    shield_color = (40, 60, 120, 255)
    shield_trim = (200, 200, 0, 255)
    
    # Shield shape
    draw.polygon([(8, 16), (14, 16), (14, 22), (11, 26), (8, 22)], fill=shield_color, outline=shield_trim)
    
    # Save
    img.save("assets/knight.png")
    print("Knight asset saved to assets/knight.png")

if __name__ == "__main__":
    create_knight()

from PIL import Image, ImageDraw
import random

def create_character(filename, skin_color, hair_color, clothes_color, style="man"):
    width, height = 32, 32
    img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Colors
    outline = (20, 20, 20, 255)
    
    # Helper to draw a pixel rect
    def rect(x, y, w, h, color):
        draw.rectangle([x, y, x+w-1, y+h-1], fill=color)

    # Body Base
    # Legs
    rect(13, 22, 2, 6, (50, 50, 50, 255)) # Left Leg
    rect(17, 22, 2, 6, (50, 50, 50, 255)) # Right Leg
    
    # Torso
    rect(12, 14, 8, 8, clothes_color)
    
    # Arms
    rect(10, 14, 2, 7, clothes_color) # Left Arm
    rect(20, 14, 2, 7, clothes_color) # Right Arm
    
    # Hands
    rect(10, 21, 2, 2, skin_color)
    rect(20, 21, 2, 2, skin_color)

    # Head
    rect(13, 6, 6, 7, skin_color)
    
    # Face details (simple eyes)
    rect(14, 8, 1, 1, (0, 0, 0, 255))
    rect(17, 8, 1, 1, (0, 0, 0, 255))
    
    # Hair
    if style == "man":
        rect(13, 5, 6, 2, hair_color) # Top
        rect(12, 6, 1, 3, hair_color) # Side L
        rect(19, 6, 1, 3, hair_color) # Side R
    elif style == "woman":
        rect(13, 5, 6, 2, hair_color) # Top
        rect(12, 6, 1, 8, hair_color) # Long Side L
        rect(19, 6, 1, 8, hair_color) # Long Side R
        rect(13, 4, 6, 1, hair_color) # Top Puff
        
    # Save
    img.save(filename)
    print(f"Saved {filename}")

if __name__ == "__main__":
    # Innkeeper (Woman, fair skin, brown hair, green dress)
    create_character("assets/npc_innkeeper.png", (255, 220, 177, 255), (139, 69, 19, 255), (34, 139, 34, 255), "woman")
    
    # Guardian (Man, dark skin, black hair, grey armor/clothes)
    create_character("assets/npc_guardian.png", (141, 85, 36, 255), (20, 20, 20, 255), (100, 100, 100, 255), "man")
    
    # Companion (Man, olive skin, blonde hair, blue clothes)
    create_character("assets/npc_companion.png", (224, 172, 105, 255), (218, 165, 32, 255), (65, 105, 225, 255), "man")
    
    # Trader (Woman, fair skin, red hair, purple clothes)
    create_character("assets/npc_trader.png", (255, 224, 189, 255), (165, 42, 42, 255), (128, 0, 128, 255), "woman")

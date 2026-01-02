from PIL import Image, ImageDraw
import os
import math
import random

def create_directory(path):
    if not os.path.exists(path):
        os.makedirs(path)

def generate_tent():
    # 64x64 Tent
    img = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Tent Body (Triangle-ish)
    # Front face
    draw.polygon([(32, 10), (10, 54), (54, 54)], fill=(200, 190, 160, 255), outline=(100, 90, 70, 255))
    
    # Open flap
    draw.polygon([(32, 10), (26, 54), (38, 54)], fill=(50, 40, 30, 255)) # Dark interior
    
    # Side shading (fake 3d)
    draw.line([(32, 10), (54, 54)], fill=(150, 140, 110, 255), width=2)
    
    img.save("assets/camp_tent.png")
    print("Generated assets/camp_tent.png")

def generate_bedroll():
    # 32x32 Bedroll
    img = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Rolled cylinder
    draw.rectangle([6, 10, 26, 22], fill=(100, 50, 50, 255), outline=(50, 20, 20, 255))
    # Straps
    draw.line([10, 10, 10, 22], fill=(200, 200, 200, 255), width=1)
    draw.line([22, 10, 22, 22], fill=(200, 200, 200, 255), width=1)
    
    img.save("assets/camp_bedroll.png")
    print("Generated assets/camp_bedroll.png")

def generate_fire_sheet():
    # 4 frames of 32x32 fire = 128x32
    sheet = Image.new('RGBA', (128, 32), (0, 0, 0, 0))
    
    for i in range(4):
        frame = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
        draw = ImageDraw.Draw(frame)
        
        # Base logs
        draw.line([(8, 28), (24, 28)], fill=(100, 70, 40, 255), width=3)
        draw.line([(10, 26), (22, 30)], fill=(90, 60, 30, 255), width=2)
        
        # Flame layers
        center_x = 16
        base_y = 26
        
        # Random fluctuation
        h_var = random.randint(0, 4)
        
        # Outer Orange
        points = [
            (center_x - 6 + random.randint(-1,1), base_y),
            (center_x - 8 + random.randint(-1,1), base_y - 8),
            (center_x, base_y - 18 - h_var),
            (center_x + 8 + random.randint(-1,1), base_y - 8),
            (center_x + 6 + random.randint(-1,1), base_y)
        ]
        draw.polygon(points, fill=(255, 100, 0, 200))
        
        # Inner Yellow
        points_y = [
            (center_x - 3, base_y),
            (center_x - 4, base_y - 6),
            (center_x, base_y - 12 - h_var),
            (center_x + 4, base_y - 6),
            (center_x + 3, base_y)
        ]
        draw.polygon(points_y, fill=(255, 200, 0, 255))
        
        sheet.paste(frame, (i * 32, 0))
        
    sheet.save("assets/camp_fire_sheet.png")
    print("Generated assets/camp_fire_sheet.png")

if __name__ == "__main__":
    generate_tent()
    generate_bedroll()
    generate_fire_sheet()

import os
import re
import tkinter as tk
from tkinter import messagebox
from PIL import Image, ImageTk

TILES_H_PATH = 'src/engine/Tiles.h'
IMAGES_DIR = 'assets/isometric tileset/separated images/'

class TileNamer:
    def __init__(self, root):
        self.root = root
        self.root.title("Pixel Gate - Tile Namer")
        
        self.tile_names = {}
        self.load_current_names()
        
        self.image_files = sorted([f for f in os.listdir(IMAGES_DIR) if f.startswith('tile_') and f.endswith('.png')])
        self.current_index = 0
        
        # UI Elements
        self.img_label = tk.Label(root)
        self.img_label.pack(pady=10)
        
        self.index_label = tk.Label(root, text="")
        self.index_label.pack()
        
        self.name_entry = tk.Entry(root, width=30)
        self.name_entry.pack(pady=5)
        self.name_entry.bind('<Return>', lambda e: self.next_tile())
        
        btn_frame = tk.Frame(root)
        btn_frame.pack(pady=10)
        
        tk.Button(btn_frame, text="Prev", command=self.prev_tile).pack(side=tk.LEFT, padx=5)
        tk.Button(btn_frame, text="Next", command=self.next_tile).pack(side=tk.LEFT, padx=5)
        tk.Button(btn_frame, text="Save Tiles.h", command=self.save_names).pack(side=tk.LEFT, padx=5)
        
        self.update_tile()

    def load_current_names(self):
        if not os.path.exists(TILES_H_PATH):
            return
        with open(TILES_H_PATH, 'r') as f:
            content = f.read()
            matches = re.findall(r'static const int ([A-Z0-9_]+)\s*=\s*(\d+);', content)
            for name, idx in matches:
                self.tile_names[int(idx)] = name

    def update_tile(self):
        filename = self.image_files[self.current_index]
        tile_idx = int(re.search(r'tile_(\d+)', filename).group(1))
        
        # Load and display image (scaled up for visibility)
        img = Image.open(os.path.join(IMAGES_DIR, filename))
        img = img.resize((128, 128), Image.NEAREST)
        self.photo = ImageTk.PhotoImage(img)
        self.img_label.config(image=self.photo)
        
        self.index_label.config(text=f"Tile Index: {tile_idx} ({self.current_index + 1}/{len(self.image_files)})")
        
        current_name = self.tile_names.get(tile_idx, "")
        self.name_entry.delete(0, tk.END)
        self.name_entry.insert(0, current_name)
        self.name_entry.focus_set()

    def next_tile(self):
        self.save_current_name()
        if self.current_index < len(self.image_files) - 1:
            self.current_index += 1
            self.update_tile()

    def prev_tile(self):
        self.save_current_name()
        if self.current_index > 0:
            self.current_index -= 1
            self.update_tile()

    def save_current_name(self):
        filename = self.image_files[self.current_index]
        tile_idx = int(re.search(r'tile_(\d+)', filename).group(1))
        name = self.name_entry.get().strip().upper()
        if name:
            self.tile_names[tile_idx] = name
        elif tile_idx in self.tile_names:
            del self.tile_names[tile_idx]

    def save_names(self):
        self.save_current_name()
        with open(TILES_H_PATH, 'w') as f:
            f.write("#pragma once\n\n")
            f.write("namespace PixelsEngine {\n\n")
            f.write("    namespace Tiles {\n")
            
            # Sort by index for cleanliness
            for idx in sorted(self.tile_names.keys()):
                name = self.tile_names[idx]
                f.write(f"        static const int {name:<20} = {idx};\n")
                
            f.write("    }\n\n")
            f.write("}\n")
        messagebox.showinfo("Saved", f"Saved {len(self.tile_names)} tiles to Tiles.h")

if __name__ == "__main__":
    root = tk.Tk()
    app = TileNamer(root)
    root.mainloop()

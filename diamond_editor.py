import tkinter as tk
from tkinter import filedialog
from PIL import Image, ImageTk, ImageDraw
import os

class DiamondEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("Isometric Diamond Editor (32x32)")
        
        self.tile_size = 32
        self.scale = 20
        self.grid = [[0 for _ in range(self.tile_size)] for _ in range(self.tile_size)]
        self.bg_image = None
        self.bg_photo = None
        
        # UI Layout
        self.canvas = tk.Canvas(root, width=self.tile_size * self.scale, height=self.tile_size * self.scale, bg="white")
        self.canvas.pack(side=tk.LEFT, padx=10, pady=10)
        
        self.canvas.bind("<B1-Motion>", self.paint)
        self.canvas.bind("<Button-1>", self.paint)
        self.canvas.bind("<Button-3>", self.erase)
        
        controls = tk.Frame(root)
        controls.pack(side=tk.RIGHT, fill=tk.Y, padx=10, pady=10)
        
        tk.Button(controls, text="Load Tile Image", command=self.load_image).pack(fill=tk.X, pady=5)
        tk.Button(controls, text="Clear Canvas", command=self.clear_grid).pack(fill=tk.X, pady=5)
        tk.Button(controls, text="Fill Standard Diamond", command=self.fill_standard).pack(fill=tk.X, pady=5)
        tk.Button(controls, text="Export Mask (mask.png)", command=self.export_mask).pack(fill=tk.X, pady=5)
        
        self.info_label = tk.Label(controls, text="Left Click: Draw\nRight Click: Erase", justify=tk.LEFT)
        self.info_label.pack(pady=20)

        self.draw_grid()

    def load_image(self):
        file_path = filedialog.askopenfilename(initialdir="assets/isometric tileset/separated images")
        if file_path:
            self.bg_image = Image.open(file_path).convert("RGBA").resize((self.tile_size * self.scale, self.tile_size * self.scale), Image.NEAREST)
            self.update_canvas()

    def paint(self, event):
        x, y = event.x // self.scale, event.y // self.scale
        if 0 <= x < self.tile_size and 0 <= y < self.tile_size:
            self.grid[y][x] = 1
            self.update_canvas()

    def erase(self, event):
        x, y = event.x // self.scale, event.y // self.scale
        if 0 <= x < self.tile_size and 0 <= y < self.tile_size:
            self.grid[y][x] = 0
            self.update_canvas()

    def clear_grid(self):
        self.grid = [[0 for _ in range(self.tile_size)] for _ in range(self.tile_size)]
        self.update_canvas()

    def fill_standard(self):
        """Fills a standard 2:1 diamond centered in the tile."""
        self.clear_grid()
        center_y = 15 # Centered roughly in the 32x32
        for y in range(8, 24):
            # Width of diamond at this y
            dist_from_center = abs(y - 15.5)
            half_width = (8 - dist_from_center) * 2
            
            start_x = int(16 - half_width)
            end_x = int(16 + half_width)
            for x in range(start_x, end_x):
                if 0 <= x < 32:
                    self.grid[y][x] = 1
        self.update_canvas()

    def draw_grid(self):
        for i in range(self.tile_size + 1):
            self.canvas.create_line(i * self.scale, 0, i * self.scale, self.tile_size * self.scale, fill="#ccc")
            self.canvas.create_line(0, i * self.scale, self.tile_size * self.scale, i * self.scale, fill="#ccc")

    def update_canvas(self):
        self.canvas.delete("all")
        if self.bg_image:
            self.bg_photo = ImageTk.PhotoImage(self.bg_image)
            self.canvas.create_image(0, 0, anchor=tk.NW, image=self.bg_photo)
        
        self.draw_grid()
        
        for y in range(self.tile_size):
            for x in range(self.tile_size):
                if self.grid[y][x]:
                    x1, y1 = x * self.scale, y * self.scale
                    x2, y2 = x1 + self.scale, y1 + self.scale
                    self.canvas.create_rectangle(x1, y1, x2, y2, fill="red", stipple="gray50", outline="")

    def export_mask(self):
        img = Image.new("L", (self.tile_size, self.tile_size), 0)
        pixels = img.load()
        for y in range(self.tile_size):
            for x in range(self.tile_size):
                if self.grid[y][x]:
                    pixels[x, y] = 255
        img.save("diamond_mask.png")
        print("Mask exported to diamond_mask.png")

if __name__ == "__main__":
    root = tk.Tk()
    app = DiamondEditor(root)
    root.mainloop()

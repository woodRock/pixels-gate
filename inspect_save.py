
try:
    with open('savegame.dat', 'r') as f:
        print(f.read())
except Exception as e:
    print(f"Error reading savegame.dat: {e}")

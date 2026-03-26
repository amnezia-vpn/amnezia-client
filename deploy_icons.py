import os
from PIL import Image

src_path = r"client\images\Лого впн отрисованный.png"
if not os.path.exists(src_path):
    print(f"Source image not found: {src_path}")
    exit(1)

img = Image.open(src_path).convert("RGBA")

# Android mipmap targets
android_sizes = {
    "mdpi": 48,
    "hdpi": 72,
    "xhdpi": 96,
    "xxhdpi": 144,
    "xxxhdpi": 192,
}

android_base = r"client\android\res"
for density, size in android_sizes.items():
    folder = os.path.join(android_base, f"mipmap-{density}")
    if not os.path.exists(folder):
        os.makedirs(folder)
    
    resized = img.resize((size, size), Image.Resampling.LANCZOS)
    resized.save(os.path.join(folder, "icon.png"), "PNG")
    resized.save(os.path.join(folder, "icon_round.png"), "PNG")
    print(f"Saved Android {density} ({size}x{size})")

# iOS AppIcon targets
ios_sizes = [20, 29, 40, 50, 57, 58, 60, 72, 76, 80, 87, 100, 114, 120, 144, 152, 167, 180, 1024]
ios_folder = r"client\ios\app\Media.xcassets\AppIcon.appiconset"

if not os.path.exists(ios_folder):
    os.makedirs(ios_folder)

for size in ios_sizes:
    resized = img.resize((size, size), Image.Resampling.LANCZOS)
    resized.save(os.path.join(ios_folder, f"{size}.png"), "PNG")
    print(f"Saved iOS AppIcon {size}x{size}")

print("Icon deployment complete!")

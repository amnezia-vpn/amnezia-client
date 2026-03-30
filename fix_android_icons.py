import os
from PIL import Image

src_path = r"client\images\Лого впн отрисованный.png"
if not os.path.exists(src_path):
    print("Source image not found")
    exit(1)

img = Image.open(src_path).convert("RGBA")
datas = img.getdata()

# Ensure we create a white silhouette based on alpha channel for Android notifications
newData = []
for item in datas:
    # item is (R, G, B, A)
    # The notification icon should be completely white with varying alpha
    newData.append((255, 255, 255, item[3]))

img.putdata(newData)
resized = img.resize((96, 96), Image.Resampling.LANCZOS)

out_folder = r"client\android\res\drawable"
if not os.path.exists(out_folder):
    os.makedirs(out_folder)

out_path = os.path.join(out_folder, "ic_fblink_round.png")
resized.save(out_path, "PNG")

# Remove the XML file so the PNG is used instead
xml_path = os.path.join(out_folder, "ic_fblink_round.xml")
if os.path.exists(xml_path):
    os.remove(xml_path)
    print("Removed ic_fblink_round.xml")

print(f"Successfully generated silhouette PNG at {out_path}")

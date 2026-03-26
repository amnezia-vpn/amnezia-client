import os
import glob

base_path = r"client\android\src\com\fblink\vpn\*.kt"
files = glob.glob(base_path)

replacements = {
    "AmneziaApplication": "FBLinkApplication",
    "AmneziaTileService": "FBLinkTileService",
    "AmneziaVpnService": "FBLinkService"
}

for filepath in files:
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    modified = content
    for old, new in replacements.items():
        modified = modified.replace(old, new)
        
    if modified != content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(modified)
        print(f"Updated {filepath}")

print("Replacement complete.")

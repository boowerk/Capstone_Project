import re
import os

folder_path = r"d:\Unreal Projects\Capstone_Project\Project_Eden\Content\Characters\UEFN_Mannequin\Animations\MotionMatchingData\ChooserTables"
assets = [
    "CHT_MM_MaskMan_Root.uasset",
    "CHT_MM_MaskMan_Idle.uasset",
    "CHT_MM_MaskMan_Run.uasset",
    "CHT_MM_MaskMan_Sprint.uasset",
    "CHT_MM_MaskMan_InAir.uasset"
]

def analyze_asset_strings_with_offsets(file_path):
    with open(file_path, 'rb') as f:
        data = f.read()
        
    results = []
    
    # 1. Search for PSD and CHT asset paths / references
    # Both ASCII and UTF-16
    for match in re.finditer(b'/Game/[a-zA-Z0-9_/.-]+', data):
        path = match.group(0).decode('ascii', errors='ignore')
        if 'PSD_' in path or 'CHT_' in path:
            results.append((match.start(), path))
            
    for match in re.finditer(b'(?:/\\x00G\\x00a\\x00m\\x00e\\x00/[a-zA-Z0-9_/.-]\\x00)+', data):
        path = match.group(0).decode('utf-16le', errors='ignore')
        if 'PSD_' in path or 'CHT_' in path:
            results.append((match.start(), path))
            
    # 2. Also search for raw asset names (like PSD_Relaxed_...) without path
    for match in re.finditer(b'(?:PSD_|CHT_)[a-zA-Z0-9_]+', data):
        name = match.group(0).decode('ascii', errors='ignore')
        results.append((match.start(), name))
        
    for match in re.finditer(b'(?:P\\x00S\\x00D\\x00_|C\\x00H\\x00T\\x00_)[a-zA-Z0-9_\\x00]+', data):
        name = match.group(0).decode('utf-16le', errors='ignore').replace('\x00', '')
        results.append((match.start(), name))

    # Remove duplicates but keep first occurrence offset
    seen = set()
    unique_results = []
    for offset, val in results:
        # Normalize: get only basename if it is a full path, to keep it clean
        base = val.split('/')[-1] if '/' in val else val
        if base not in seen and base != '':
            seen.add(base)
            unique_results.append((offset, base, val))
            
    # Sort by offset
    unique_results.sort(key=lambda x: x[0])
    return unique_results

for asset in assets:
    path = os.path.join(folder_path, asset)
    if os.path.exists(path):
        print(f"\n==================================================")
        print(f" Asset: {asset} (Size: {os.path.getsize(path)} bytes)")
        print(f"==================================================")
        
        matches = analyze_asset_strings_with_offsets(path)
        for idx, (offset, base, full) in enumerate(matches):
            print(f" [{idx + 1}] Offset: {offset:5d} | Name: {base}")
            if '/' in full:
                print(f"     Path: {full}")
    else:
        print(f"Asset not found: {path}")

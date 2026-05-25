import struct
import re
import os

def extract_float_property(data, prop_name):
    # Search for prop_name followed by 'FloatProperty' or similar
    # In UE uasset, it usually has the property name string, and nearby 'FloatProperty'
    # Since uasset stores strings in NameMap, the serialised data blocks may have:
    # PropName FName index + Type FName index + Size (int32) + ...
    # But often, the property data itself in the export table contains the serialized property.
    # In tagged property serialization (which blueprints use):
    # [FName Name] [FName Type] [int32 Size] [int32 Index] [Value]
    # Under standard UE format: FName is 8 bytes (int32 StringIndex, int32 Number)
    # So we can search for the FName index of Name and Type in the NameMap,
    # or simply search for ASCII representation if the asset stores it or we can find it.
    # Actually, uasset has a NameMap at the beginning which contains all ASCII/UTF16 strings.
    # The serialization data itself uses 8-byte FName (index into NameMap).
    # Therefore, we must first parse the NameMap to find the indices of our target property and 'FloatProperty'.
    pass

def parse_uasset_names(data):
    # uasset format summary:
    # Legacy File Summary has:
    # - Tag (0x9E2A83C1)
    # - NameCount and NameOffset
    if len(data) < 192:
        return []
    
    # Read NameCount and NameOffset from header
    # For UE4/UE5, the header structure:
    # Tag is at 0, NameCount is at 120, NameOffset is at 124 (or similar, depending on version)
    # Let's search for Tag 0x9E2A83C1 to verify it's a uasset
    tag = struct.unpack('<I', data[0:4])[0]
    if tag != 0x9E2A83C1:
        return []
        
    # Standard UE5 header offsets:
    # NameCount is typically at offset 112 or 120. Let's find it.
    # Let's search the uasset binary for known float properties and print around them.
    return []

# A more robust heuristic: just find all occurrences of float patterns or use python's string search
# Let's write a simple script that searches for specific strings in the uasset and prints their binary context.
def search_bytes_context(file_path, search_str):
    if not os.path.exists(file_path):
        print(f"File not found: {file_path}")
        return
    with open(file_path, 'rb') as f:
        data = f.read()
    
    print(f"\n--- Searching '{search_str}' in {os.path.basename(file_path)} ---")
    # Search for ASCII
    idx = data.find(search_str.encode('ascii'))
    if idx != -1:
        print(f"ASCII found at index {idx}")
        # Print next 64 bytes in hex and ascii
        ctx = data[idx:idx+128]
        print(f"Hex: {ctx.hex()}")
        print(f"Decoded: {ctx.decode('ascii', errors='ignore')}")
    else:
        print("ASCII not found")
        
    # Search for UTF-16LE
    utf16_str = search_str.encode('utf-16le')
    idx_u = data.find(utf16_str)
    if idx_u != -1:
        print(f"UTF-16 found at index {idx_u}")
        ctx = data[idx_u:idx_u+128]
        print(f"Hex: {ctx.hex()}")
    else:
        print("UTF-16 not found")

# Test with TargetArmLength
search_bytes_context(r"D:\Unreal Projects\Capstone_Project\Project_Eden\Content\Characters\PlayerCharacter\BP_GP_PlayerCharacter.uasset", "TargetArmLength")
search_bytes_context(r"D:\Unreal Projects\GameAnimationSample\Content\Blueprints\SandboxCharacter_CMC.uasset", "TargetArmLength")

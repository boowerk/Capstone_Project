import struct
import os

def read_header(file_path):
    if not os.path.exists(file_path):
        print("File not found")
        return
    with open(file_path, 'rb') as f:
        data = f.read(1024)
        
    tag = struct.unpack('<I', data[0:4])[0]
    print(f"Tag: {hex(tag)}")
    
    # Let's print out potential NameCount and NameOffset positions
    # In UE5, let's look at offsets from 90 to 160
    for offset in range(80, 200, 4):
        val1, val2 = struct.unpack('<II', data[offset:offset+8])
        # NameCount is usually ~100-3000, NameOffset is usually ~192-2000
        if 50 < val1 < 10000 and 100 < val2 < 5000000:
            print(f"Candidate at offset {offset}: Count={val1}, Offset={val2}")
        if 50 < val2 < 10000 and 100 < val1 < 5000000:
            print(f"Candidate at offset {offset} (reversed): Offset={val1}, Count={val2}")

print("--- BP_GP_PlayerCharacter ---")
read_header(r"D:\Unreal Projects\Capstone_Project\Project_Eden\Content\Characters\PlayerCharacter\BP_GP_PlayerCharacter.uasset")
print("--- SandboxCharacter_CMC ---")
read_header(r"D:\Unreal Projects\GameAnimationSample\Content\Blueprints\SandboxCharacter_CMC.uasset")

import struct
import os
import sys

def find_name_count_offset(data):
    sig = struct.unpack_from("<I", data, 0)[0]
    if sig != 0x9E2A83C1:
        raise ValueError("Not a valid UE uasset file")

    folder_start = data.find(b"/Game/")
    if folder_start == -1:
        folder_start = data.find(b"/Script/")
    if folder_start == -1:
        raise ValueError("Cannot find FolderName in header")

    folder_len_off = folder_start - 4
    folder_len = struct.unpack_from("<i", data, folder_len_off)[0]
    folder_end = folder_start + folder_len

    for test_off in range(folder_end, min(folder_end + 200, len(data) - 8), 4):
        count = struct.unpack_from("<i", data, test_off)[0]
        offset_val = struct.unpack_from("<i", data, test_off + 4)[0]
        if (1 <= count <= 10000 and 100 <= offset_val <= len(data)
                and offset_val + 4 < len(data)):
            test_len = struct.unpack_from("<i", data, offset_val)[0]
            if 1 <= test_len <= 512:
                return count, offset_val

    raise ValueError("Cannot find NameCount/NameOffset in header")

def parse_name_map(data, name_offset, name_count):
    name_map = []
    offset = name_offset

    for i in range(name_count):
        if offset >= len(data):
            break

        str_len = struct.unpack_from("<i", data, offset)[0]

        if 1 <= str_len <= 1024:
            try:
                s = data[offset + 4:offset + 4 + str_len - 1].decode("ascii")
                if data[offset + 4 + str_len - 1] == 0:
                    name_map.append(s)
                    after = offset + 4 + str_len

                    for skip in [0, 4, 2]:
                        nl = (struct.unpack_from("<i", data, after + skip)[0]
                              if after + skip + 4 <= len(data) else 999999)
                        if 1 <= nl <= 1024 or (-1024 <= nl < 0):
                            offset = after + skip
                            break
                    else:
                        offset = after
                    continue
            except (UnicodeDecodeError, IndexError):
                pass

        if -1024 <= str_len < 0:
            abs_len = -str_len
            byte_len = abs_len * 2
            try:
                s = data[offset + 4:offset + 4 + byte_len - 2].decode("utf-16-le")
                name_map.append(s)
                offset += 4 + byte_len
                continue
            except (UnicodeDecodeError, IndexError):
                pass

        break

    return name_map

def scan_properties(file_path, target_properties):
    if not os.path.exists(file_path):
        return f"File not found: {file_path}"
        
    with open(file_path, "rb") as f:
        data = f.read()
        
    try:
        name_count, name_offset = find_name_count_offset(data)
        name_map = parse_name_map(data, name_offset, name_count)
    except Exception as e:
        return f"Error parsing name map: {e}"
        
    results = {}
    
    # Pre-find string indices
    prop_indices = {}
    for prop in target_properties:
        if prop in name_map:
            prop_indices[prop] = name_map.index(prop)
            
    float_prop_idx = name_map.index("FloatProperty") if "FloatProperty" in name_map else -1
    bool_prop_idx = name_map.index("BoolProperty") if "BoolProperty" in name_map else -1
    struct_prop_idx = name_map.index("StructProperty") if "StructProperty" in name_map else -1
    vector_idx = name_map.index("Vector") if "Vector" in name_map else -1
    rotator_idx = name_map.index("Rotator") if "Rotator" in name_map else -1
    
    for prop, p_idx in prop_indices.items():
        # Search for Name FName in data: p_idx (4 bytes) + 0 (4 bytes)
        target_bin = struct.pack("<ii", p_idx, 0)
        pos = 0
        found_instances = []
        
        while True:
            pos = data.find(target_bin, pos)
            if pos == -1:
                break
                
            # Verify if this is indeed a Property Header
            # Standard property header has Type FName next
            if pos + 16 <= len(data):
                type_idx, type_num = struct.unpack_from("<ii", data, pos + 8)
                if type_num == 0 and type_idx < len(name_map):
                    type_str = name_map[type_idx]
                    
                    if type_str == "FloatProperty" and pos + 29 <= len(data):
                        size = struct.unpack_from("<i", data, pos + 16)[0]
                        arr_idx = struct.unpack_from("<i", data, pos + 20)[0]
                        has_guid = data[pos + 24]
                        
                        val_offset = pos + 25
                        if has_guid != 0:
                            val_offset += 16
                            
                        if val_offset + 4 <= len(data):
                            float_val = struct.unpack_from("<f", data, val_offset)[0]
                            found_instances.append(f"Float: {float_val}")
                            
                    elif type_str == "BoolProperty" and pos + 26 <= len(data):
                        size = struct.unpack_from("<i", data, pos + 16)[0]
                        arr_idx = struct.unpack_from("<i", data, pos + 20)[0]
                        bool_val = data[pos + 24] != 0
                        has_guid = data[pos + 25] != 0
                        found_instances.append(f"Bool: {bool_val}")
                        
                    elif type_str == "StructProperty" and pos + 48 <= len(data):
                        # Struct properties have additional metadata
                        size = struct.unpack_from("<i", data, pos + 16)[0]
                        arr_idx = struct.unpack_from("<i", data, pos + 20)[0]
                        struct_type_idx = struct.unpack_from("<i", data, pos + 24)[0]
                        
                        struct_type = name_map[struct_type_idx] if struct_type_idx < len(name_map) else "Unknown"
                        val_offset = pos + 49 # FName struct_type (8) + GUID (16) + HasGUID (1)
                        
                        if struct_type == "Vector" and val_offset + 12 <= len(data):
                            x, y, z = struct.unpack_from("<fff", data, val_offset)
                            found_instances.append(f"Vector(X={x:.2f}, Y={y:.2f}, Z={z:.2f})")
                        elif struct_type == "Rotator" and val_offset + 12 <= len(data):
                            pitch, yaw, roll = struct.unpack_from("<fff", data, val_offset)
                            found_instances.append(f"Rotator(P={pitch:.2f}, Y={yaw:.2f}, R={roll:.2f})")
                        else:
                            found_instances.append(f"Struct: {struct_type} (size {size})")
                            
            pos += 4
            
        if found_instances:
            # Often there are multiple instances in a BP uasset (CDO, instance template, blueprint variable nodes)
            # Typically, the last instance or the one with specific defaults represents the CDO (Class Default Object)
            results[prop] = found_instances
            
    return results

target_props = [
    # CharacterMovement
    "GravityScale", "MaxWalkSpeed", "MaxAcceleration", "JumpZVelocity", "AirControl", 
    "BrakingDecelerationFalling", "BrakingDecelerationWalking", "GroundFriction",
    "BrakingFrictionFactor", "RotationRate",
    "bOrientRotationToMovement", "bUseControllerDesiredRotation", "bUseControllerRotationYaw",
    # SpringArm
    "TargetArmLength", "bEnableCameraLag", "CameraLagSpeed", "bEnableCameraRotationLag",
    "CameraRotationLagSpeed", "SocketOffset", "TargetOffset", "bUsePawnControlRotation"
]

print("=== BP_GP_PlayerCharacter (Capstone) ===")
res_capstone = scan_properties(
    r"D:\Unreal Projects\Capstone_Project\Project_Eden\Content\Characters\PlayerCharacter\BP_GP_PlayerCharacter.uasset",
    target_props
)
for k, v in res_capstone.items():
    print(f"  {k}: {v}")
    
print("\n=== SandboxCharacter_CMC (Sample) ===")
res_sample_cmc = scan_properties(
    r"D:\Unreal Projects\GameAnimationSample\Content\Blueprints\SandboxCharacter_CMC.uasset",
    target_props
)
for k, v in res_sample_cmc.items():
    print(f"  {k}: {v}")

print("\n=== SandboxCharacter_Mover (Sample) ===")
res_sample_mover = scan_properties(
    r"D:\Unreal Projects\GameAnimationSample\Content\Blueprints\SandboxCharacter_Mover.uasset",
    target_props
)
for k, v in res_sample_mover.items():
    print(f"  {k}: {v}")

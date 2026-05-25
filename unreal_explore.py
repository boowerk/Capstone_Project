import unreal
import traceback
import os

log_file_path = r"d:\Unreal Projects\Capstone_Project\chooser_dump_result.txt"

def run_dump():
    with open(log_file_path, "w", encoding="utf-8") as out:
        out.write("=== CHOOSER PATH VERIFICATION START ===\n")
        try:
            # 1. Print physical file metadata
            target_file = r"D:\Unreal Projects\Capstone_Project\Project_Eden\Content\Characters\UEFN_Mannequin\Animations\MotionMatchingData\ChooserTables\CHT_MM_MaskMan_Root.uasset"
            if os.path.exists(target_file):
                stat = os.stat(target_file)
                out.write(f"Physical File: {target_file}\n")
                out.write(f"  Size: {stat.st_size} bytes\n")
                out.write(f"  Modified: {stat.st_mtime}\n\n")
            else:
                out.write(f"Physical File not found at: {target_file}\n\n")
            
            # 2. Load asset and verify package path
            asset_path = '/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/ChooserTables/CHT_MM_MaskMan_Root'
            asset = unreal.EditorAssetLibrary.load_asset(asset_path)
            
            if not asset:
                out.write("FAILED: Unreal loaded asset is None!\n")
                return
                
            out.write(f"Successfully loaded: {asset.get_name()}\n")
            out.write(f"Class: {asset.get_class().get_name()}\n")
            
            # Get package and check actual filename loaded by Unreal
            package = asset.get_package()
            package_name = package.get_name()
            out.write(f"Package Name: {package_name}\n")
            
            # Resolve to physical filename using PackageName API
            try:
                resolved_filename = unreal.PackageName.long_package_name_to_filename(package_name, ".uasset")
                out.write(f"Resolved package filename: {resolved_filename}\n")
                # Make it absolute
                abs_resolved = os.path.abspath(resolved_filename)
                out.write(f"Absolute resolved filename: {abs_resolved}\n")
            except Exception as e_res:
                out.write(f"Failed to resolve filename: {e_res}\n")
                
            out.write("\n=== CHOOSER PATH VERIFICATION END ===\n")
            
        except Exception as e:
            out.write(f"\nFATAL ERROR DURING VERIFICATION: {e}\n")
            traceback.print_exc(file=out)

try:
    run_dump()
except Exception as e_outer:
    with open(log_file_path, "a", encoding="utf-8") as out:
        out.write(f"\nOUTER FATAL ERROR: {e_outer}\n")

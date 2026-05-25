import unreal

def tune_air_movement():
    bp_path = '/Game/Characters/PlayerCharacter/BP_GP_PlayerCharacter'
    bp_asset = unreal.EditorAssetLibrary.load_asset(bp_path)
    if not bp_asset:
        print("Failed to load blueprint:", bp_path)
        return

    # 컴포넌트 템플릿 탐색하여 CharacterMovement 속성 직접 덮어쓰기
    templates = bp_asset.get_editor_property('component_templates')
    modified = False
    for comp in templates:
        if comp.get_class().get_name() == 'CharacterMovementComponent':
            comp.set_editor_property('AirControl', 0.25)
            comp.set_editor_property('BrakingDecelerationFalling', 1500.0)
            print(f"Successfully tuned Component: {comp.get_name()}")
            modified = True
            
    if modified:
        unreal.EditorAssetLibrary.save_loaded_asset(bp_asset)
        print("CharacterMovement Air parameters applied to BP asset and saved successfully!")
    else:
        print("CharacterMovementComponent template not found in BP_GP_PlayerCharacter.")

if __name__ == '__main__':
    tune_air_movement()

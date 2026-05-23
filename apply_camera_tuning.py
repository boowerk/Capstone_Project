import unreal

def tune_camera():
    bp_path = '/Game/Characters/PlayerCharacter/BP_GP_PlayerCharacter'
    bp_asset = unreal.EditorAssetLibrary.load_asset(bp_path)
    if not bp_asset:
        print("Failed to load blueprint:", bp_path)
        return

    # 컴포넌트 템플릿 탐색하여 SpringArm 속성 직접 덮어쓰기
    templates = bp_asset.get_editor_property('component_templates')
    modified = False
    for comp in templates:
        if comp.get_class().get_name() == 'SpringArmComponent':
            comp.set_editor_property('TargetArmLength', 380.0)
            comp.set_editor_property('bEnableCameraLag', True)
            comp.set_editor_property('CameraLagSpeed', 12.0)
            comp.set_editor_property('bEnableCameraRotationLag', True)
            comp.set_editor_property('CameraRotationLagSpeed', 15.0)
            comp.set_editor_property('SocketOffset', unreal.Vector(0.0, 50.0, 20.0))
            comp.set_editor_property('TargetOffset', unreal.Vector(0.0, 0.0, 0.0))
            print(f"Successfully tuned Component Template: {comp.get_name()}")
            modified = True
            
    if modified:
        unreal.EditorAssetLibrary.save_loaded_asset(bp_asset)
        print("Camera tuning applied to BP asset and saved successfully!")
    else:
        print("SpringArmComponent template not found in BP_GP_PlayerCharacter.")

if __name__ == '__main__':
    tune_camera()

import unreal
import os

def tune_pose_search_schema():
    # 1. PSS 에셋 경로 지정 (제자리 회전 턴인플레이스 스키마)
    pss_path = '/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Schemas/PSS_Relaxed_StandTurn'
    
    # 2. 에셋 로드
    pss = unreal.EditorAssetLibrary.load_asset(pss_path)
    
    if not pss:
        print(f"FAILED: Cannot load Pose Search Schema at {pss_path}")
        return
        
    print(f"Successfully loaded Schema: {pss.get_name()}")
    
    modified = False
    
    # 3. 스키마 채널 중 Trajectory Channel 탐색 및 Heading 가중치 튜닝
    for channel in pss.channels:
        channel_class_name = channel.get_class().get_name()
        if "Trajectory" in channel_class_name:
            print(f"Found Trajectory Channel: {channel.get_name()} ({channel_class_name})")
            
            # samples 배열 내 각 샘플링 지점의 가중치 수정
            if hasattr(channel, 'samples'):
                new_samples = []
                for idx, sample in enumerate(channel.samples):
                    # Yaw 회전 각도(Heading) 가중치를 대폭 늘려 (3.0) 카메라 정렬 추종성 최대화
                    sample.weight = 3.0
                    new_samples.append(sample)
                    print(f"  Sample [{idx}] Time:{sample.time_offset}s -> Weight set to {sample.weight}")
                
                # Struct 복사본 재할당 (Unreal Python 구조체 바인딩 반영)
                channel.set_editor_property('samples', new_samples)
                modified = True

    # 4. 수정 성공 시 에셋 저장
    if modified:
        success = unreal.EditorAssetLibrary.save_loaded_asset(pss)
        if success:
            print("SUCCESS: PSS_Relaxed_StandTurn 에셋 수정 및 저장 완료! 이제 모션매칭 엔진이 자체적으로 카메라 Yaw 정렬을 완벽히 흡수합니다.")
        else:
            print("FAILED: Failed to save the modified PSS asset.")
    else:
        print("INFO: No Trajectory Channel samples were modified.")

if __name__ == "__main__":
    tune_pose_search_schema()

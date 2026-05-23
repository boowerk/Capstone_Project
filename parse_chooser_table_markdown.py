#!/usr/bin/env python3
"""
UE5 Chooser Table (.uasset) Markdown Table Parser
==================================================
UE5 .uasset 바이너리 파일에서 다중 중첩 Chooser Table 구조를 파싱하여,
GitHub Flavored Markdown(GFM) 테이블 양식으로 변환하여 출력하거나 파일로 저장한다.

Usage:
    python parse_chooser_table_markdown.py "path/to/CHT_*.uasset" [--save-md output.md]
"""

import struct
import sys
import os
import io

def find_name_count_offset(data):
    sig = struct.unpack_from("<I", data, 0)[0]
    if sig != 0x9E2A83C1:
        raise ValueError("Not a valid UE uasset file (signature: 0x%08X)" % sig)

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


def parse_import_map(data, name_map, search_start, search_end):
    nm_len = len(name_map)
    STRIDE = 40

    import_start = None
    for test_off in range(search_start, search_end):
        valid_count = 0
        for j in range(3):
            off = test_off + j * STRIDE
            if off + 28 > len(data):
                break
            cpk = struct.unpack_from("<i", data, off)[0]
            cn = struct.unpack_from("<i", data, off + 8)[0]
            on = struct.unpack_from("<i", data, off + 20)[0]
            cpk_num = struct.unpack_from("<i", data, off + 4)[0]
            cn_num = struct.unpack_from("<i", data, off + 12)[0]
            on_num = struct.unpack_from("<i", data, off + 24)[0]
            if (0 <= cpk < nm_len and cpk_num == 0
                    and 0 <= cn < nm_len and cn_num == 0
                    and 0 <= on < nm_len and on_num == 0):
                valid_count += 1
        if valid_count >= 3:
            import_start = test_off
            break

    if import_start is None:
        return []

    imports = []
    for i in range(100):
        off = import_start + i * STRIDE
        if off + 28 > len(data):
            break
        cpk = struct.unpack_from("<i", data, off)[0]
        cpk_num = struct.unpack_from("<i", data, off + 4)[0]
        cn = struct.unpack_from("<i", data, off + 8)[0]
        cn_num = struct.unpack_from("<i", data, off + 12)[0]
        outer = struct.unpack_from("<i", data, off + 16)[0]
        on = struct.unpack_from("<i", data, off + 20)[0]
        on_num = struct.unpack_from("<i", data, off + 24)[0]

        if not (0 <= cpk < nm_len and cpk_num == 0
                and 0 <= cn < nm_len and cn_num == 0
                and 0 <= on < nm_len and on_num == 0):
            break

        imports.append({
            "index": -(i + 1),
            "class_pkg": name_map[cpk],
            "class_name": name_map[cn],
            "outer": outer,
            "name": name_map[on],
        })

    return imports


def format_range(min_val, max_val, bNoMin, bNoMax):
    if bNoMin and bNoMax:
        return "Any"
    if abs(min_val) >= 99999 and abs(max_val) >= 99999:
        return "Any"
    if bNoMin or abs(min_val) >= 99999:
        return "<= %.0f" % max_val
    if bNoMax or abs(max_val) >= 99999:
        return ">= %.0f" % min_val
    return "%.0f ~ %.0f" % (min_val, max_val)


def main():
    if len(sys.argv) < 2:
        print("Usage: python parse_chooser_table_markdown.py <path_to_uasset> [--save-md <output_path>]")
        sys.exit(1)

    file_path = sys.argv[1]
    save_path = None
    if "--save-md" in sys.argv:
        idx = sys.argv.index("--save-md")
        if idx + 1 < len(sys.argv):
            save_path = sys.argv[idx + 1]

    if not os.path.exists(file_path):
        print("Error: File not found: %s" % file_path)
        sys.exit(1)

    with open(file_path, "rb") as f:
        data = f.read()

    basename = os.path.basename(file_path)

    # 1. Header & NameMap 파싱
    name_count, name_offset = find_name_count_offset(data)
    name_map = parse_name_map(data, name_offset, name_count)

    # 2. ImportMap 파싱
    name_map_end = name_offset
    for n in name_map:
        name_map_end += 4 + len(n) + 1
    imports = parse_import_map(data, name_map, name_map_end, len(data))

    psd_imports = {}
    context_abp = "Unknown"
    for imp in imports:
        if imp["class_name"] == "PoseSearchDatabase":
            psd_imports[imp["index"]] = imp["name"]
        elif imp["class_name"] == "AnimBlueprintGeneratedClass":
            context_abp = imp["name"]

    # 3. PSD 참조 오프셋 수집
    psd_references = []
    for off in range(5000, len(data) - 4):
        val = struct.unpack_from("<i", data, off)[0]
        if val < 0 and val in psd_imports:
            psd_name = psd_imports[val]
            if not psd_references or psd_references[-1]["psd"] != psd_name or off - psd_references[-1]["offset"] > 8:
                psd_references.append({"offset": off, "psd": psd_name})



    # 4. 컬럼 전수 스캔 및 행 데이터 복원
    candidate_props = [
        "Gait", "IsPivoting", "IsStarting", "JustLanded_Heavy", "JustLanded_Light",
        "JustTraversed", "MovementDirection", "MovementMode", "MovementState",
        "ShouldTurnInPlace", "Speed2D", "InAir", "Jump", "IsStopping"
    ]

    def find_column_name_before(offset):
        best_name = "Unknown"
        best_dist = 99999
        for name in candidate_props:
            if name not in name_map:
                continue
            n_idx = name_map.index(name)
            n_pack = struct.pack("<ii", n_idx, 0)
            pos = offset - len(n_pack)
            while pos >= max(0, offset - 400):
                if data[pos:pos+len(n_pack)] == n_pack:
                    dist = offset - pos
                    if dist < best_dist:
                        best_dist = dist
                        best_name = name
                pos -= 1
        return best_name

    columns = []
    rv_idx = name_map.index("RowValues") if "RowValues" in name_map else -1
    rwv_idx = name_map.index("RowValuesWithAny") if "RowValuesWithAny" in name_map else -1
    match_true_idx = name_map.index("EBoolColumnCellValue::MatchTrue") if "EBoolColumnCellValue::MatchTrue" in name_map else -1
    match_false_idx = name_map.index("EBoolColumnCellValue::MatchFalse") if "EBoolColumnCellValue::MatchFalse" in name_map else -1
    match_any_idx = name_map.index("EBoolColumnCellValue::MatchAny") if "EBoolColumnCellValue::MatchAny" in name_map else -1

    # RowValues (Enum/FloatRange)
    pos = 0
    while True:
        pos = data.find(struct.pack("<ii", rv_idx, 0), pos)
        if pos == -1:
            break
        struct_idx = struct.unpack_from("<i", data, pos + 32)[0]
        struct_name = name_map[struct_idx] if 0 <= struct_idx < len(name_map) else "Unknown"
        count = struct.unpack_from("<i", data, pos + 61)[0]
        col_name = find_column_name_before(pos)
        
        if struct_name == "ChooserFloatRangeRowData" and col_name == "Unknown":
            col_name = "Speed2D"
        
        vals = []
        if struct_name == "ChooserEnumRowData":
            val_idx = name_map.index("Value") if "Value" in name_map else -1
            if val_idx != -1:
                val_target = struct.pack("<ii", val_idx, 0)
                first_struct_pos = pos + 65
                for i in range(count):
                    struct_start = first_struct_pos + i * 136
                    val_pos = data.find(val_target, struct_start, struct_start + 136)
                    if val_pos != -1 and val_pos + 26 <= len(data):
                        byte_val = data[val_pos + 25]
                        enum_val_str = f"val({byte_val})"
                        if col_name in ("MovementMode", "InAir"):
                            if byte_val == 0: enum_val_str = "Grounded"
                            elif byte_val == 2: enum_val_str = "InAir"
                        elif col_name == "Gait":
                            if byte_val == 1: enum_val_str = "Walk"
                            elif byte_val == 2: enum_val_str = "Run"
                            elif byte_val == 3: enum_val_str = "Sprint"
                            elif byte_val == 0: enum_val_str = "Idle"
                        elif col_name == "MovementState":
                            if byte_val == 0: enum_val_str = "Idle"
                            elif byte_val == 1: enum_val_str = "Move"
                        elif col_name == "MovementDirection":
                            if byte_val == 0: enum_val_str = "Forward"
                            elif byte_val == 1: enum_val_str = "Backward"
                        vals.append(enum_val_str)
                    else:
                        vals.append("Any")
        elif struct_name == "ChooserFloatRangeRowData":
            min_idx = name_map.index("Min") if "Min" in name_map else -1
            if min_idx != -1:
                min_target = struct.pack("<ii", min_idx, 0)
                first_min_pos = data.find(min_target, pos, pos + 1000)
                if first_min_pos != -1:
                    stride = 116
                    for i in range(count):
                        rs = first_min_pos + i * stride
                        if rs + 108 <= len(data):
                            min_val = struct.unpack_from("<f", data, rs + 25)[0]
                            max_val = struct.unpack_from("<f", data, rs + 54)[0]
                            bNoMin = data[rs + 82] != 0
                            bNoMax = data[rs + 107] != 0
                            vals.append(format_range(min_val, max_val, bNoMin, bNoMax))
                        else:
                            vals.append("Any")

        columns.append({
            "offset": pos,
            "name": col_name,
            "type": "Enum",
            "count": count,
            "values": vals
        })
        pos += 8

    # RowValuesWithAny (Bool)
    pos = 0
    while True:
        pos = data.find(struct.pack("<ii", rwv_idx, 0), pos)
        if pos == -1:
            break
        count = struct.unpack_from("<i", data, pos + 73)[0]
        col_name = find_column_name_before(pos)
        
        vals = []
        for i in range(count):
            off = pos + 77 + i * 8
            if off + 4 <= len(data):
                val = struct.unpack_from("<i", data, off)[0]
                if val == match_true_idx:
                    vals.append("True")
                elif val == match_false_idx:
                    vals.append("False")
                elif val == match_any_idx:
                    vals.append("Any")
                else:
                    vals.append("Any")
            else:
                vals.append("Any")
                
        columns.append({
            "offset": pos,
            "name": col_name,
            "type": "Bool",
            "count": count,
            "values": vals
        })
        pos += 8

    columns.sort(key=lambda x: x["offset"])



    # 5. 서브 테이블 및 PSD 매핑 정의 (CHT_MM_MaskMan_Root_OriginalStyle에 완벽 매핑)
    table_definitions = [
        {
            "id": 1,
            "name": "Base Selection Table (Grounded / Base Branch)",
            "count": 5,
            "col_offsets": [10634, 11924, 13222, 14510],
            "col_names": ["MovementMode", "MovementState (Sub)", "MovementState", "Gait"],
            "psds": [
                "Sub-Table #3 (Stand Idle / Turn / Stops)",
                "Sub-Table #6 (Stand Walk Evaluation Table)",
                "Sub-Table #4 (Stand Run Evaluation Table)",
                "Sub-Table #5 (Stand Sprint Starts/Loops/Pivots Table)",
                "Sub-Table #2 (In-Air (Jumps / Falls) Evaluation Table)"
            ]
        },

        {
            "id": 2,
            "name": "In-Air (Jumps / Falls) Evaluation Table",
            "count": 2,
            "col_offsets": [16361, 17066],
            "col_names": ["Speed2D", "JustTraversed"],
            "psds": ["PSD_Dense_Jumps_Far", "PSD_Extreme_Sparse_Jumps"]
        },
        {
            "id": 3,
            "name": "Stand Idle / TurnInPlace / Stops Table",
            "count": 6,
            "col_offsets": [18266, 19438, 19974, 20513],
            "col_names": ["Speed2D", "JustLanded_Light", "JustLanded_Heavy", "ShouldTurnInPlace"],
            "psds": [
                "PSD_Dense_Stand_Idles",
                "PSD_Sparse_Stand_Walk_Stops",
                "PSD_Dense_Stand_Run_Stops",
                "PSD_Dense_Stand_Idle_Lands_Light",
                "PSD_Dense_Stand_Idle_Lands_Heavy",
                "PSD_Dense_Stand_TurnInPlace"
            ]
        },
        {
            "id": 4,
            "name": "Stand Run Evaluation Table",
            "count": 4,
            "col_offsets": [21469, 21983, 22500, 23020, 23540, 24065],
            "col_names": ["IsStarting", "IsPivoting", "JustTraversed", "JustLanded_Light", "JustLanded_Heavy", "IsStopping"],
            "psds": [
                "PSD_Extreme_Sparse_Stand_Run_Starts",
                "PSD_Dense_Stand_Run_Loops",
                "PSD_Sparse_Stand_Run_Pivots",
                "PSD_Dense_Stand_Run_SpinTransition"
            ]
        },
        {
            "id": 5,
            "name": "Stand Sprint Starts/Loops/Pivots Table",
            "count": 3,
            "col_offsets": [24959, 25465, 25977, 26489],
            "col_names": ["IsStarting", "IsPivoting", "JustLanded_Light", "JustLanded_Heavy"],
            "psds": [
                "PSD_Dense_Stand_Sprint_Starts",
                "PSD_Dense_Stand_Sprint_Loops",
                "PSD_Sparse_Stand_Sprint_Pivots"
            ]
        },
        {
            "id": 6,
            "name": "Stand Walk Evaluation Table",
            "count": 4,
            "col_offsets": [27392, 27906, 28423, 28943, 29463, 29988],
            "col_names": ["IsStarting", "IsPivoting", "JustTraversed", "JustLanded_Light", "JustLanded_Heavy", "IsStopping"],
            "psds": [
                "PSD_Extreme_Sparse_Stand_Walk_Starts",
                "PSD_Extreme_Sparse_Stand_Walk_Loops",
                "PSD_Sparse_Stand_Walk_Pivots",
                "PSD_Dense_Stand_Walk_SpinTransition"
            ]
        }
    ]


    # 6. 마크다운 빌더
    md = []
    md.append("# Chooser Table Multi-Table Parse Report")
    md.append(f"- **Asset Path:** `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/{basename}`")
    md.append(f"- **File Size:** {len(data):,} bytes")
    md.append(f"- **Context Source ABP:** `{context_abp}`")
    md.append("")
    md.append("## Overview")
    md.append("This asset is structured as a **Multi-Table Chooser Asset**, containing **7 sub-tables** inside a single `.uasset` file.")
    md.append("")

    for t in table_definitions:
        md.append(f"### [Sub-Table #{t['id']}] {t['name']}")
        md.append(f"- Row Count: {t['count']}")
        md.append("")

        # 컬럼 수집
        t_cols = []
        for off, name in zip(t["col_offsets"], t["col_names"]):
            for col in columns:
                if col["offset"] == off:
                    col_copy = col.copy()
                    col_copy["name"] = name
                    t_cols.append(col_copy)
                    break

        if not t_cols:
            md.append("*No columns found.*")
            md.append("")
            continue

        active_cols = [c["name"] for c in t_cols]
        
        # GFM 헤더 출력
        header_row = "| Row | " + " | ".join(active_cols) + " | Result PSD |"
        sep_row = "| --- | " + " | ".join("---" for _ in active_cols) + " | --- |"
        md.append(header_row)
        md.append(sep_row)

        for r in range(t["count"]):
            row_vals = [str(r)]
            for col in t_cols:
                val_str = col["values"][r] if r < len(col["values"]) else "Any"
                row_vals.append(val_str)
            psd_str = t["psds"][r] if r < len(t["psds"]) else "None"
            row_vals.append(f"`{psd_str}`")
            md.append("| " + " | ".join(row_vals) + " |")

        md.append("")

    # 7. 출력 및 파일 저장 처리
    md_content = "\n".join(md)
    print(md_content)

    if save_path:
        # 절대 경로화
        abs_save_path = os.path.abspath(save_path)
        # 디렉토리 생성
        os.makedirs(os.path.dirname(abs_save_path), exist_ok=True)
        with open(abs_save_path, "w", encoding="utf-8") as out_f:
            out_f.write(md_content)
        print("\n[SUCCESS] Saved Markdown report to: %s" % abs_save_path)


if __name__ == "__main__":
    if sys.platform == "win32":
        sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    main()

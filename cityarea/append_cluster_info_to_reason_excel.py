import pandas as pd
from openpyxl import load_workbook
from openpyxl.styles import Alignment, Font

from classify_and_merge_excel_simplified_reason import COMPONENT_ZH_MAP

SOURCE_XLSX = 'data/city/Beijing/best_csv/城市地块分类_北京_reason.xlsx'
HIERARCHY_CSV = 'data/city/Beijing/best_csv/cluster_semantic_hierarchy_view_v1.csv'
OUTPUT_XLSX = 'data/city/Beijing/best_csv/城市地块分类_北京_reason_with_cluster.xlsx'

REV_COMPONENT_MAP = {v: k for k, v in COMPONENT_ZH_MAP.items()}


def to_english_key(label: str) -> str:
    return REV_COMPONENT_MAP.get(str(label).strip(), str(label).strip())


def first_two_items(signature: str):
    parts = []
    for part in str(signature).split('|'):
        part = part.strip()
        if ':' in part:
            key, value = part.split(':', 1)
            parts.append((key.strip(), value.strip()))
    return tuple(parts[:2])


def build_cluster_index(csv_path: str):
    df = pd.read_csv(csv_path)
    index = {}
    for _, row in df.iterrows():
        sig = row['top_signature']
        if pd.isna(sig) or str(sig).strip() == '':
            continue
        index[first_two_items(sig)] = {
            'cluster_id': int(row['cluster_id']),
            'top_signature': str(sig),
        }
    return index


def match_excel_rows(source_xlsx: str, hierarchy_csv: str, output_xlsx: str):
    index = build_cluster_index(hierarchy_csv)
    wb = load_workbook(source_xlsx)
    ws = wb.active

    ws.cell(row=1, column=5, value='cluster_id')
    ws.cell(row=1, column=6, value='top_signature')

    matched = 0
    unmatched_rows = []

    for r in range(2, ws.max_row + 1):
        comp = ws.cell(r, 4).value
        if not comp:
            continue

        parts = []
        for part in str(comp).split('|'):
            part = part.strip()
            if ':' in part:
                key, value = part.split(':', 1)
                parts.append((to_english_key(key), value.strip()))
        key = tuple(parts[:2])
        info = index.get(key)

        if info is None:
            unmatched_rows.append(r)
            continue

        ws.cell(r, 5, info['cluster_id'])
        ws.cell(r, 6, info['top_signature'])
        matched += 1

    # Style headers and align new columns
    for c in range(1, 7):
        ws.cell(1, c).font = Font(bold=True)
        ws.cell(1, c).alignment = Alignment(horizontal='center', vertical='center', wrap_text=True)

    for r in range(2, ws.max_row + 1):
        ws.cell(r, 5).alignment = Alignment(horizontal='center', vertical='center')
        ws.cell(r, 6).alignment = Alignment(horizontal='left', vertical='center', wrap_text=True)

    ws.column_dimensions['E'].width = 12
    ws.column_dimensions['F'].width = 95

    wb.save(output_xlsx)
    print(f'Matched rows: {matched}/{ws.max_row - 1}')
    if unmatched_rows:
        print('Unmatched rows:', unmatched_rows)
    print(f'Saved: {output_xlsx}')


if __name__ == '__main__':
    match_excel_rows(SOURCE_XLSX, HIERARCHY_CSV, OUTPUT_XLSX)

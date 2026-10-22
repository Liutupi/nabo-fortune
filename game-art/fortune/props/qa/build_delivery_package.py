"""Package existing art files and verify every ZIP entry against disk."""
import hashlib
import json
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

ART = Path(__file__).resolve().parents[2]
QA = ART / 'props' / 'qa'
ZIP = QA / 'static-art-delivery.zip'


def digest(data):
    return hashlib.sha256(data).hexdigest()


files = []
for folder in ('props', 'tarot'):
    for p in (ART / folder).rglob('*'):
        if p.is_file() and 'qa' not in p.relative_to(ART / folder).parts:
            files.append(p)
for rel in (
    'props/qa/verification.json',
    'props/qa/visual-review.json',
    'props/qa/props-native-overview.png',
    'props/qa/props-detail-check.png',
    'props/qa/verify_static_delivery.py',
    'props/qa/protected-input-hashes.json',
    'props/qa/build_delivery_package.py',
    'tarot/qa/12-identity-upright-check.png',
    'costume/export-art.ps1',
    'costume/final.png',
    'costume/final-transparent.png',
    'costume/final-palette.json',
    'reference/nabo-idle.png',
):
    files.append(ART / rel)
files = sorted(set(files), key=lambda p: p.relative_to(ART).as_posix())
assert all(p.is_file() for p in files)

payload = []
for p in files:
    raw = p.read_bytes()
    payload.append({
        'path': p.relative_to(ART).as_posix(),
        'bytes': len(raw),
        'sha256': digest(raw),
    })
manifest = {
    'schema_version': 1,
    'date': '2026-10-22',
    'root': 'game-art/fortune',
    'note': 'Payload list excludes this manifest to avoid a self-referential hash.',
    'files': payload,
}
manifest_path = QA / 'package-file-manifest.json'
manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
files.append(manifest_path)

with ZipFile(ZIP, 'w', compression=ZIP_DEFLATED, compresslevel=6) as z:
    for p in files:
        z.write(p, p.relative_to(ART).as_posix())

with ZipFile(ZIP) as z:
    names = z.namelist()
    assert len(names) == len(set(names)) == len(files)
    assert z.testzip() is None
    for p in files:
        rel = p.relative_to(ART).as_posix()
        assert digest(z.read(rel)) == digest(p.read_bytes()), rel
    props = json.loads((ART / 'props/asset-manifest.json').read_text(encoding='utf-8'))['assets']
    cards = json.loads((ART / 'tarot/asset-manifest.json').read_text(encoding='utf-8'))['cards']
    assets = props + cards
    assert len(props) == 29 and len(cards) == 22
    for a in assets:
        for variant in ('native', 'master_4x'):
            record = a[variant]
            assert digest(z.read(record['path'])) == record['sha256'], record['path']

archive_hash = digest(ZIP.read_bytes())
(QA / 'static-art-delivery.zip.sha256.txt').write_text(
    f'{archive_hash}  {ZIP.name}\n', encoding='utf-8'
)
result = {
    'date': '2026-10-22',
    'archive': 'props/qa/' + ZIP.name,
    'bytes': ZIP.stat().st_size,
    'sha256': archive_hash,
    'file_count': len(files),
    'payload_file_count': len(payload),
    'all_zip_entries_read_back_and_hash_match': True,
    'zip_crc_test_passed': True,
    'native_asset_count': len(assets),
    'master_4x_asset_count': len(assets),
    'all_native_and_master_hashes_match_asset_manifests': True,
    'new_generation_source_pngs': 33,
    'existing_shell_source_included': True,
    'videos_included': False,
}
(QA / 'package-verification.json').write_text(
    json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8'
)
print(json.dumps(result, ensure_ascii=False, indent=2))

from pathlib import Path
import hashlib

base = Path('data/audio')
renamed = []
skipped = []

for path in sorted(base.glob('*.wav')):
    name = path.name
    if len(name) <= 31:
        continue

    stem = path.stem
    suffix = path.suffix
    digest = hashlib.md5(name.encode('utf-8')).hexdigest()[:4]
    short_stem = stem[:22]
    new_name = f"{short_stem}_{digest}{suffix}"

    target = path.with_name(new_name)
    if target.exists():
        skipped.append((name, new_name, 'target exists'))
        continue

    path.rename(target)
    renamed.append((name, new_name))

print('Renamed:')
for old, new in renamed:
    print(f'  {old} -> {new}')

if not renamed:
    print('  (none)')

if skipped:
    print('Skipped:')
    for old, new, reason in skipped:
        print(f'  {old} -> {new} ({reason})')

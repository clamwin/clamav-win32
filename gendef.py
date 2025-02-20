#!/usr/bin/env python3
from pathlib import Path

def main():
    defs_dir = Path(__file__).parent / 'defs'

    ordinal = 1000
    lines = []

    for def_file in defs_dir.iterdir():
        _, name = def_file.stem.split('_', 1)
        if 'noname' in name:
            name, _ = name.split('_noname')
            noname = True
        else:
            noname = False

        lines.append((f'\n; {name}', None))

        for line in def_file.read_text().split('\n'):
            parts = line.strip().split()
            if not parts or parts[0] == ';':
                continue

            parts = [x.strip() for x in parts]
            line = ''
            if noname:
                line += f'@{ordinal} NONAME'
                ordinal += 1

            if len(parts) > 1:
                line += f' {" ".join(parts[1:])}'

            lines.append((parts[0], line))

    maxlen = max([len(line[0]) for line in lines]) + 1
    print('; AUTOMATIC GENERATED - DO NOT EDIT\nEXPORTS')
    for left, right in lines:
        padding = maxlen - len(left)
        print(left + (' ' * padding) + (right or ''))

if __name__ == '__main__':
    main()

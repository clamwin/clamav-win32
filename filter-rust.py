#!/usr/bin/env python3
import re
import subprocess
import sys

BLACKLIST_SYMBOLS = [
    'CreateSymbolicLinkW',
    'FindFirstFileExW',
    'GetFileInformationByHandleEx',
    'GetFinalPathNameByHandleW',
    'GetTimeZoneInformationForYear',
    'ReOpenFile',
    'SetFileInformationByHandle',
    'GetHostNameW',
]

BLACKLIST_MODULES = [
    'api-ms-win-core-synch-l1-2-0',
    'bcryptprimitives',
]

re_modules = re.compile(
    r'\n+(?P<name>.*?):\s+file format .*?SYMBOL TABLE:\n(?P<symbols>.*?)\n\n',
    re.M | re.S)

re_symbols = re.compile(
    r'^\[\s*(?P<index>\d+)\]'
    r'\(sec\s+(?P<section>\d+)\)'
    r'\(fl\s+(?P<flags>0x[0-9A-Fa-f]+)\)'
    r'\(ty\s+(?P<type>\d+)\)'
    r'\(scl\s+(?P<sclass>\d+)\)\s+'
    r'\(nx\s+(?P<aux>\d+)\)\s+'
    r'(?P<value>0x[0-9A-Fa-f]+)\s+'
    r'(?P<name>.+)$',
    re.M
)


def main():
    if len(sys.argv) != 4:
        raise Exception(f'Usage: {sys.argv[0]} objdump_exe ar_exe archive')

    objdump = sys.argv[1]
    ar = sys.argv[2]
    archive = sys.argv[3]

    data = subprocess.check_output([objdump, '-t', archive], text=True)
    modules = set()

    for module in re_modules.finditer(data):
        module_name = module.group('name')

        for match in BLACKLIST_MODULES:
            if match.lower() in module_name.lower():
                modules.add(module_name)

        for symbol in re_symbols.finditer(module.group('symbols')):
            if symbol.group('section') not in ('1', '5'):
                continue
            if symbol.group('sclass') != '2':
                continue
            name = symbol.group('name')
            if name.startswith('_ZN'):
                continue
            for match in BLACKLIST_SYMBOLS:
                if match in name:
                    modules.add(module_name)
                    break

    print(f'[filter-rust] running (twice):\n{ar} d {archive} {" ".join(modules)}')
    subprocess.check_call([ar, 'd', archive, *modules])
    subprocess.check_call([ar, 'd', archive, *modules])


if __name__ == '__main__':
    main()

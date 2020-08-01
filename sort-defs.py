import sys

def main():
    symbols = []
    with open(sys.argv[1], 'r') as f:
        line = f.readline().strip()
        symbol, first_ordinal = line.split()
        first_ordinal = int(first_ordinal[1:])
        symbols.append(symbol)
        for line in f:
            symbols.append(line.strip().split()[0])

    symbols.sort()
    ordinalpos = ((len(max(symbols, key=len)) / 4) + 1) * 4

    sorted_symbols = []
    for ordinal, symbol in enumerate(symbols, first_ordinal):
        sorted_symbols.append(symbol + (' ' * (ordinalpos - len(symbol))) + '@' + str(ordinal) + '\n')

    with open(sys.argv[2], 'w') as f:
        f.writelines(sorted_symbols)

if __name__ == '__main__':
    main()

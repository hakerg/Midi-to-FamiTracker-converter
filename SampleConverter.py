data = """
DPCM : 69 35 37 80 01 23 E8 1C FE D7 FF DE 38 78 E2 94 8B C6 61 46 30 80 C3 C7 7E 7D CF E8 6F 1C 78 5C
DPCM : C4 00 3E 0E 32 43 9E E3 3C 8F B7 3F BC 38 8C 1B 18 38 8E 0E F0 4D 74 1C 9D DF F1 99 B6 E0 B4 61
DPCM : 18 73 01 3F 70 1D 9E D1 6D 9C E7 9A 32 8F 25 54 2E 06 C7 C1 E3 D6 78 1C BC C3 3B 31 E7 E8 70 1C
DPCM : 3C 58 9C E4 F0 9C 39 0F 76 F2 69 C3 71 39 38 72 9C 58 65 AC 46 4E 1F A7 F8 D1 36 9C 1C 87 27 C3
DPCM : 19 8E 39 C6 B9 E4 B4 F2 CC B2 4C CD CA AC 31 8E E1 32 4D B9 32 AE 72 6C 56 56 B5 54 55 55 AA 95
DPCM : 96 AA AA AA A6 AA 66 5A 59 55 55 55 55 95 65 9A AA AA AA AA AA 5A A6 59 55 A5 95 56 5A 55 55 55
DPCM : 55
"""

# Przygotuj dane do przekształcenia, usuwając niepotrzebne fragmenty i dzieląc na poszczególne bajty
data = data.replace("DPCM :", "").replace("\n", "").split()

# Przekształć bajty na właściwe formatowanie
cpp_data = ", ".join([f"0x{byte}" for byte in data])

# Wyświetl wynik
print(f"{{ {cpp_data} }}")
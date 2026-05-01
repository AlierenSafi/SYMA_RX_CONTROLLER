#!/usr/bin/env python3
"""
MegaJoy.hex isim degistirme araci
Intel HEX formatindaki string descriptor'lari degistirir
"""

import sys
import struct

def intel_hex_to_binary(hex_file):
    """Intel HEX dosyasini binary'ye cevir"""
    data = bytearray()
    segment = 0
    
    with open(hex_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line[0] != ':':
                continue
                
            byte_count = int(line[1:3], 16)
            address = int(line[3:7], 16)
            record_type = int(line[7:9], 16)
            
            if record_type == 0x00:  # Data record
                addr = (segment << 16) + address
                # Gerekirse genislet
                while len(data) < addr + byte_count:
                    data.append(0xFF)
                
                for i in range(byte_count):
                    data[addr + i] = int(line[9 + i*2:11 + i*2], 16)
                    
            elif record_type == 0x02:  # Extended segment address
                segment = int(line[9:13], 16)
            elif record_type == 0x04:  # Extended linear address
                segment = int(line[9:13], 16)
                
    return data

def binary_to_intel_hex(data, bytes_per_line=16):
    """Binary'yi Intel HEX formatina cevir"""
    lines = []
    
    # Start code
    lines.append(":020000040000FA")
    
    for addr in range(0, len(data), bytes_per_line):
        chunk = data[addr:addr + bytes_per_line]
        
        record = f":{len(chunk):02X}{addr & 0xFFFF:04X}00"
        checksum = len(chunk) + ((addr >> 8) & 0xFF) + (addr & 0xFF)
        
        for byte in chunk:
            record += f"{byte:02X}"
            checksum += byte
            
        checksum = (-checksum) & 0xFF
        record += f"{checksum:02X}"
        lines.append(record)
        
    # End of file
    lines.append(":00000001FF")
    
    return "\n".join(lines)

def find_unicode_string(data, search_text):
    """Unicode (UTF-16LE) string ara"""
    # UTF-16LE olarak encode et
    encoded = search_text.encode('utf-16le')
    
    positions = []
    start = 0
    while True:
        pos = data.find(encoded, start)
        if pos == -1:
            break
        positions.append(pos)
        start = pos + 1
        
    return positions

def replace_unicode_string(data, old_text, new_text):
    """Unicode string'i degistir (ayni uzunlukta olmali)"""
    old_encoded = old_text.encode('utf-16le')
    new_encoded = new_text.encode('utf-16le')
    
    if len(old_encoded) != len(new_encoded):
        print(f"UYARI: Uzunluklar farkli! Eski: {len(old_encoded)}, Yeni: {len(new_encoded)}")
        print(f"Eski text: '{old_text}' ({len(old_text)} karakter)")
        print(f"Yeni text: '{new_text}' ({len(new_text)} karakter)")
        return None
        
    positions = find_unicode_string(data, old_text)
    
    if not positions:
        print(f"'{old_text}' bulunamadi!")
        return None
        
    print(f"'{old_text}' {len(positions)} yerde bulundu:")
    for pos in positions:
        print(f"  Pozisyon: {pos} (0x{pos:X})")
        
    # Degistir
    result = bytearray(data)
    for pos in positions:
        result[pos:pos+len(old_encoded)] = new_encoded
        
    return bytes(result)

def main():
    if len(sys.argv) < 2:
        input_file = "MegaJoy.hex"
    else:
        input_file = sys.argv[1]
        
    output_file = "AES_Controller.hex"
    
    print(f"Dosya: {input_file}")
    print("Intel HEX -> Binary ceviriliyor...")
    
    binary = intel_hex_to_binary(input_file)
    print(f"Binary boyut: {len(binary)} bytes")
    
    # Stringleri ara
    print("\nAraniyor...")
    
    # "UnoJoy Joystick" ara
    positions = find_unicode_string(binary, "UnoJoy Joystick")
    if positions:
        print(f"\n'UnoJoy Joystick' bulundu: {positions}")
    else:
        print("\n'UnoJoy Joystick' bulunamadi, diger stringler deneniyor...")
        # Diger olasi stringler
        for test in ["UnoJoy", "Joystick", "MegaJoy", "OpenChord"]:
            pos = find_unicode_string(binary, test)
            if pos:
                print(f"  '{test}' bulundu: {pos}")
    
    # Uretici adi
    positions = find_unicode_string(binary, "OpenChord X RMIT Exertion Games Lab")
    if positions:
        print(f"\nUretici bulundu: {positions}")
    
    # Simdi degistir - ayni uzunlukta olmali
    print("\n--- DEGISIKLIKLER ---")
    
    # "UnoJoy Joystick" -> "AES Controller  " (16 karakter = 32 byte)
    modified = replace_unicode_string(binary, "UnoJoy Joystick", "AES Controller ")
    
    if modified:
        # "OpenChord X RMIT Exertion Games Lab" -> "AES Electronics                  "
        # 37 karakter - uzunluk ayni kalmali
        modified = replace_unicode_string(modified, "OpenChord X RMIT Exertion Games Lab", "AES Electronics                    ")
        
        if modified:
            print("\nIntel HEX formatina ceviriliyor...")
            hex_output = binary_to_intel_hex(modified)
            
            with open(output_file, 'w') as f:
                f.write(hex_output)
                
            print(f"\nBasarili! Yeni dosya: {output_file}")
            print("Bu dosyayi ATmega16U2'ye yukleyin.")
        else:
            print("Uretici adi degistirilemedi!")
    else:
        print("\nIsim degistirilemedi! Manuel hex edit gerekli.")
        print("\nAlternatif: HxD editor ile dosyayi acin ve")
        print("'UnoJoy Joystick' yazan yerleri arayip degistirin.")

if __name__ == "__main__":
    main()

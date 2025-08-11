#!/usr/bin/env python3
import sys

def hex_file_to_c_array(filename):
    """
    Reads a text file containing hex bytes separated by whitespace
    and prints a C array declaration.
    """
    with open(filename, 'r') as f:
        # Split on any whitespace (spaces, newlines, etc.)
        hex_bytes = f.read().strip().split()

    # Format each as 0xNN
    formatted = ', '.join(f'0x{byte.upper()}' for byte in hex_bytes)

    # Output C code
    print(f"static uint8_t packetBuf[] = {{ {formatted} }};")
    print("uint32_t packetLen = sizeof(packetBuf);")

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <hexfile>")
        sys.exit(1)
    hex_file_to_c_array(sys.argv[1])

if __name__ == '__main__':
    main()

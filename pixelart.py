import cv2
import numpy as np
import argparse
import os
import sys

# Ensure terminal can print UTF-8 characters (like block elements)
if sys.stdout.encoding != 'utf-8':
    sys.stdout.reconfigure(encoding='utf-8')

def convert_image(input_path, output_path, display_ascii=True, text_output_path=None, width=128, height=64, dark_thresh=100):
    if not os.path.exists(input_path):
        print(f"Error: Input file '{input_path}' not found.")
        return

    # Load image
    img = cv2.imread(input_path)
    if img is None:
        print(f"Error: Could not load image from '{input_path}'.")
        return

    # Convert to grayscale
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    # Resize image to the target dimensions
    resized = cv2.resize(gray, (width, height), interpolation=cv2.INTER_AREA)

    # 1. Pixel Art Outlines (Canny edge detection)
    edges = cv2.Canny(resized, 100, 200)
    
    # 2. Dark Swatches
    # Threshold for darker colors. Anything darker than dark_thresh becomes solid.
    _, dark_swatches = cv2.threshold(resized, dark_thresh, 255, cv2.THRESH_BINARY_INV)
    
    # 3. Combine outlines and dark swatches
    edges = cv2.bitwise_or(edges, dark_swatches)

    # Invert to make the detected features black (0) and background white (255)
    pixel_art = cv2.bitwise_not(edges)

    # Save output image
    if output_path:
        cv2.imwrite(output_path, pixel_art)
        print(f"Saved 128x64 pixel art to '{output_path}'")

    if display_ascii or text_output_path:
        ascii_output = ""
        # We use Unicode block elements to pack 2 vertical pixels into 1 character
        # This makes the 128x64 image fit comfortably in most terminals (128x32 characters)
        for y in range(0, height, 2):
            line = ""
            for x in range(width):
                top_edge = edges[y, x] > 127
                bottom_edge = edges[y+1, x] > 127 if (y+1) < height else False
                
                # In terminal, block characters are drawn in the foreground color.
                # So drawing a block means drawing the *edge*.
                if top_edge and bottom_edge:
                    line += "█" # Full block
                elif top_edge:
                    line += "▀" # Upper half block
                elif bottom_edge:
                    line += "▄" # Lower half block
                else:
                    line += " " # Space
            ascii_output += line + "\n"
        
        if display_ascii:
            print(f"\nASCII Art Representation ({width}x{height}):\n")
            print(ascii_output)
            
        if text_output_path:
            with open(text_output_path, 'w', encoding='utf-8') as f:
                f.write(ascii_output)
            print(f"Saved ASCII art to text file '{text_output_path}'")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert an image to 128x64 B&W pixel art (outlines).")
    parser.add_argument("input", help="Path to the input image file.")
    parser.add_argument("-o", "--output", default="output_pixel_art.png", help="Path to save the output image (default: output_pixel_art.png)")
    parser.add_argument("-t", "--text-out", default=None, help="Path to save the ASCII output to a text file.")
    parser.add_argument("--no-ascii", action="store_true", help="Disable ASCII art output to terminal.")
    parser.add_argument("-d", "--dark-thresh", type=int, default=100, help="Intensity threshold (0-255) for dark swatches. Pixels darker than this become solid. Default is 100.")
    
    args = parser.parse_args()
    
    convert_image(args.input, args.output, display_ascii=not args.no_ascii, text_output_path=args.text_out, dark_thresh=args.dark_thresh)

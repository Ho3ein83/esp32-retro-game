import re
import argparse
from pathlib import Path
from PIL import Image

# === RGB888 to RGB565 ===
def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

# === Resize or Crop to Fit ===
def fit_image(img, max_w, max_h):
    w, h = img.size
    if w <= max_w and h <= max_h:
        return img
    img.thumbnail((max_w, max_h), Image.LANCZOS)
    return img

def sanitize_cpp_identifier(name):
    name = re.sub(r'\W', '_', name)
    if re.match(r'^\d', name):
        name = '_' + name
    return name

def convert_image(path, max_w, max_h, alpha_threshold=None):
    image = Image.open(path)
    if alpha_threshold is not None:
        image = image.convert("RGBA")
    else:
        image = image.convert("RGB")

    image = fit_image(image, max_w, max_h)
    width, height = image.size

    pixels = []
    for y in range(height):
        for x in range(width):
            px = image.getpixel((x, y))

            if alpha_threshold is not None:
                r, g, b, a = px
                if a <= alpha_threshold:
                    pixels.append(0x0001)
                    continue
            else:
                r, g, b = px

            pixels.append(rgb888_to_rgb565(r, g, b))


    size_bytes = len(pixels) * 2  # uint16_t
    return width, height, pixels, size_bytes

def main():
    parser = argparse.ArgumentParser(description="Convert image(s) to RGB565 C array")
    parser.add_argument("inputs", nargs="+", help="Image files or glob patterns")
    parser.add_argument("-o", "--output", help="Output header file")
    parser.add_argument("-x", "--width", type=int, default=240)
    parser.add_argument("-y", "--height", type=int, default=240)
    parser.add_argument("-m", "--merge", action="store_true", help="Merge all images into one header")
    parser.add_argument("-t", "--transparent", nargs="?", const=0, type=int, default=None, metavar="ALPHA", help="Enable transparency. Pixels with alpha <= ALPHA become transparent (default: 0)")


    args = parser.parse_args()

    # === Resolve glob patterns ===
    image_files = []
    for pattern in args.inputs:
        image_files.extend(Path(".").glob(pattern))

    image_files = [p for p in image_files if p.is_file()]
    if not image_files:
        print("❌ No matching image files found")
        return

    # === MERGE MODE ===
    if args.merge:
        output_path = Path(args.output or "images.h")

        total_bytes = 0
        image_data = []

        for img in image_files:
            w, h, pixels, size_bytes = convert_image(img, args.width, args.height, args.transparent)
            total_bytes += size_bytes

            array_name = sanitize_cpp_identifier(img.stem + "_bmp")
            image_data.append((img.name, array_name, w, h, pixels, size_bytes))

        with open(output_path, "w") as f:
            f.write(f"// Total bitmap data size: {total_bytes} bytes\n\n")

            for name, arr, w, h, pixels, size_bytes in image_data:
                f.write(f"// {name}\n")
                f.write(f"// Size: {w}x{h} ({size_bytes} bytes)\n")
                f.write(f"const uint16_t {arr}[{len(pixels)}] PROGMEM = {{\n")

                for i, val in enumerate(pixels):
                    f.write(f"0x{val:04X}, ")
                    if (i + 1) % 16 == 0:
                        f.write("\n")

                f.write("\n};\n\n")

        print(f"✅ Merged {len(image_files)} images → {output_path}")

    # === SINGLE FILE MODE ===
    else:
        for img in image_files:
            output_path = Path(args.output) if args.output else img.with_suffix(".h")

            w, h, pixels, size_bytes = convert_image(img, args.width, args.height, args.transparent)
            array_name = sanitize_cpp_identifier(img.stem + "_bmp")

            with open(output_path, "w") as f:
                f.write(f"// {img.name}\n")
                f.write(f"// Size: {w}x{h} ({size_bytes} bytes)\n")
                f.write(f"const uint16_t {array_name}[{len(pixels)}] = {{\n")

                for i, val in enumerate(pixels):
                    f.write(f"0x{val:04X}, ")
                    if (i + 1) % 16 == 0:
                        f.write("\n")

                f.write("\n};\n")

            print(f"✅ {img.name} → {output_path}")

if __name__ == "__main__":
    main()


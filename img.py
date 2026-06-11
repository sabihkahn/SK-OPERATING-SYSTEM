from PIL import Image

# 1. Load your designed cake image
img = Image.open("me1.png")

# 2. Force Pillow to match adaptive colors safely using the modern configuration
img = img.convert("RGB")
img = img.quantize(colors=256, method=2) # 2 = Fast Octree quantization method

width, height = img.size

# 3. Modern way to get flat pixel data safely
pixels = list(img.getdata())

# 4. Open the file 'imgdata.txt' for writing
with open("imgdata.txt", "w") as f:
    f.write(f"// VGA Optimized Asset ({width}x{height})\n")
    f.write(f"unsigned char img_cake_pixels[{len(pixels)}] = {{\n")
    
    for i in range(0, len(pixels), width):
        row = pixels[i:i+width]
        row_str = ", ".join(str(p) for p in row)
        f.write(f"    {row_str},\n")
        
    f.write("};\n")

print("Successfully generated and saved optimized array to imgdata.txt!")
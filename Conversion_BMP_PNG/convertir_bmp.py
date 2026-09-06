from pathlib import Path
from PIL import Image

CARPETA_ENTRADA = Path("imagenes")

CARPETA_SALIDA = Path("convertidas")

archivos_bmp = list(CARPETA_ENTRADA.glob("*.bmp")) + list(CARPETA_ENTRADA.glob("*.BMP"))

print(f"Encontrados {len(archivos_bmp)} archivos BMP.")

for archivo in archivos_bmp:
    try:
        imagen = Image.open(archivo)

        archivo_salida = CARPETA_SALIDA / f"{archivo.stem}.png"

        imagen.save(archivo_salida, "PNG")

        print(f"OK: {archivo.name} -> {archivo_salida.name}")

    except Exception as e:
        print(f"ERROR: {archivo.name} -> {e}")

print("\nConversión terminada.")

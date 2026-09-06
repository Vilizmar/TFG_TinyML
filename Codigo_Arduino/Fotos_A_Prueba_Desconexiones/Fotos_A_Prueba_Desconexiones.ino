#include "SDMMCBlockDevice.h" // Multi Media Card APIs
#include "FATFileSystem.h"    // API to run operations on a FAT file system
SDMMCBlockDevice blockDevice;
mbed::FATFileSystem fileSystem("fs");

#include "mbed.h"
#include "camera.h" // Arduino Mbed Core Camera APIs
#include "himax.h"  // API to read from the Himax camera found on the Portenta Vision Shield
HM01B0 himax;
Camera cam(himax);

FrameBuffer frameBuffer; // Buffer to save the camera stream

// Settings for our setup
#define IMAGE_HEIGHT (unsigned int)240
#define IMAGE_WIDTH (unsigned int)320
#define IMAGE_MODE CAMERA_GRAYSCALE
#define BITS_PER_PIXEL (unsigned int)8
#define PALETTE_COLORS_AMOUNT (unsigned int)(pow(2, BITS_PER_PIXEL))
#define PALETTE_SIZE  (unsigned int)(PALETTE_COLORS_AMOUNT * 4) // 4 bytes = 32bit per color (3 bytes RGB and 1 byte 0x00)
#define IMAGE_PATH "/fs/image.bmp"

// Headers info
#define BITMAP_FILE_HEADER_SIZE (unsigned int)14 // For storing general information about the bitmap image file
#define DIB_HEADER_SIZE (unsigned int)40 // For storing information about the image and define the pixel format
#define HEADER_SIZE (BITMAP_FILE_HEADER_SIZE + DIB_HEADER_SIZE)

//Variables para tomar fotos en bucle y con un límite
unsigned long tiempoAnterior = 0;
int contadorFotos = 0;
bool tomarFotos = true;

void setup(){
    Serial.begin(115200);
    // while (!Serial);
    
    Serial.println("Mounting SD Card...");
    mountSDCard();
    Serial.println("SD Card mounted.");

    // Se comprueba cuántas imágenes hay guardadas ya en la SD.
    Serial.println("Buscando la última foto guardada...");
    contadorFotos = obtenerSiguienteIndice();
    Serial.print("Continuando captura desde la foto: img_");
    Serial.print(contadorFotos);
    Serial.println(".bmp");

    if (!cam.begin(CAMERA_R320x240, IMAGE_MODE, 30)){
        Serial.println("Unable to find the camera");
    }
    
    // Iniciamos el Watchdog con un límite de 10 segundos (10000 ms)
    mbed::Watchdog &watchdog = mbed::Watchdog::get_instance();
    watchdog.start(10000); 
    
    
    Serial.println("Setup completo. Iniciando capturas...");
}

void loop(){

    mbed::Watchdog::get_instance().kick();

    // Solo entra al bloque si tomarFotos es true y han pasado 5 segundos
    
    if (tomarFotos && (millis() - tiempoAnterior >= 2500)) {
        tiempoAnterior = millis(); 
        
        Serial.println("Fetching camera image...");
        unsigned char *imageData = captureImage();
        
        delay(500); 

        char rutaImagen[32];
        sprintf(rutaImagen, "/fs/img_%d.bmp", contadorFotos); 
        
        Serial.print("Saving image to SD card: ");
        Serial.println(rutaImagen);
        
        saveImage(imageData, rutaImagen);

        // Feedback visual de la toma de fotos

        digitalWrite(LEDG, LOW);  // Enciende el LED verde
        delay(100);            
        digitalWrite(LEDG, HIGH); // Apaga el LED verde

        contadorFotos++; 
        
        // Comprobación del límite de imágenes que pongamos
        if (contadorFotos >= 5000) {
            Serial.println("Desmontando tarjeta SD...");
            
            fileSystem.unmount(); // Desmontamos de forma segura
            
            Serial.println("Done. You can now remove the SD card.");
            tomarFotos = false; // Se cambia a false para que el loop deje de tomar fotos y desconectar de forma segura
        } else {
            Serial.println("Imagen guardada. Esperando 5 segundos...");
        }
    }
}


// Mount File system block
void mountSDCard(){
    int error = fileSystem.mount(&blockDevice);
    if (error){
        Serial.println("Trying to reformat...");
        int formattingError = fileSystem.reformat(&blockDevice);
        if (formattingError) {            
            Serial.println("Error: No SD Card found. Reiniciando placa...");
            delay(1000);
            NVIC_SystemReset(); // Se fuerza un reinicio por software
        }
    }
}



// Get the raw image data (8bpp grayscale)
unsigned char * captureImage(){
    if (cam.grabFrame(frameBuffer, 3000) == 0){
        return frameBuffer.getBuffer();
    } else {
        Serial.println("Error: No se pudo capturar el frame. Reiniciando placa...");
        delay(1000); // Pequeña pausa para que el mensaje Serial se envíe
        NVIC_SystemReset(); // Se fuerza un reinicio por software
    }
}

// Set the headers data
void setFileHeaders(unsigned char *bitmapFileHeader, unsigned char *bitmapDIBHeader, int fileSize){
    // Set the headers to 0
    memset(bitmapFileHeader, (unsigned char)(0), BITMAP_FILE_HEADER_SIZE);
    memset(bitmapDIBHeader, (unsigned char)(0), DIB_HEADER_SIZE);

    // File header
    bitmapFileHeader[0] = 'B';
    bitmapFileHeader[1] = 'M';
    bitmapFileHeader[2] = (unsigned char)(fileSize);
    bitmapFileHeader[3] = (unsigned char)(fileSize >> 8);
    bitmapFileHeader[4] = (unsigned char)(fileSize >> 16);
    bitmapFileHeader[5] = (unsigned char)(fileSize >> 24);
    bitmapFileHeader[10] = (unsigned char)HEADER_SIZE + PALETTE_SIZE;

    // Info header
    bitmapDIBHeader[0] = (unsigned char)(DIB_HEADER_SIZE);
    bitmapDIBHeader[4] = (unsigned char)(IMAGE_WIDTH);
    bitmapDIBHeader[5] = (unsigned char)(IMAGE_WIDTH >> 8);
    bitmapDIBHeader[8] = (unsigned char)(IMAGE_HEIGHT);
    bitmapDIBHeader[9] = (unsigned char)(IMAGE_HEIGHT >> 8);
    bitmapDIBHeader[14] = (unsigned char)(BITS_PER_PIXEL);
}

void setColorMap(unsigned char *colorMap){
    //Init the palette with zeroes
    memset(colorMap, (unsigned char)(0), PALETTE_SIZE);
    
    // Gray scale color palette, 4 bytes per color (R, G, B, 0x00)
    for (int i = 0; i < PALETTE_COLORS_AMOUNT; i++) {
        colorMap[i * 4] = i;
        colorMap[i * 4 + 1] = i;
        colorMap[i * 4 + 2] = i;
    }
}

// Save the headers and the image data into the .bmp file
void saveImage(unsigned char *imageData, const char* imagePath){
    int fileSize = BITMAP_FILE_HEADER_SIZE + DIB_HEADER_SIZE + IMAGE_WIDTH * IMAGE_HEIGHT;
    FILE *file = fopen(imagePath, "wb");

    if (file == NULL) {
        Serial.print("ERROR: No se pudo abrir la SD para guardar ");
        Serial.println(imagePath);
        return; // Salimos de la función sin intentar escribir
    }

    // Bitmap structure (Head + DIB Head + ColorMap + binary image)
    unsigned char bitmapFileHeader[BITMAP_FILE_HEADER_SIZE];
    unsigned char bitmapDIBHeader[DIB_HEADER_SIZE];
    unsigned char colorMap[PALETTE_SIZE]; // Needed for <= 8bpp grayscale bitmaps    

    setFileHeaders(bitmapFileHeader, bitmapDIBHeader, fileSize);
    setColorMap(colorMap);

    // Write the bitmap file
    fwrite(bitmapFileHeader, 1, BITMAP_FILE_HEADER_SIZE, file);
    fwrite(bitmapDIBHeader, 1, DIB_HEADER_SIZE, file);
    fwrite(colorMap, 1, PALETTE_SIZE, file);
    fwrite(imageData, 1, IMAGE_HEIGHT * IMAGE_WIDTH, file);

    fflush(file);

    // Close the file stream
    fclose(file);
}


// Busca el último número de foto guardado en la SD
int obtenerSiguienteIndice() {
    int indice = 0;
    char ruta[32];
    
    while (true) {
        sprintf(ruta, "/fs/img_%d.bmp", indice);
        FILE *file = fopen(ruta, "r");
        
        if (file) {
            // El archivo existe. Se prueba el siguiente.
            fclose(file); 
            indice++;
        } else {
             // El archivo no existe. Se empieza aqui.
            return indice; 
        }
    }
}

void countDownBlink(){
    for (int i = 0; i < 6; i++){
        digitalWrite(LEDG, i % 2);
        delay(500);
    }
    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDB, LOW);
}
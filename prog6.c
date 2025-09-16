/*
 * Author: Aidan Fernadnes
 * Clemson Login Name: Aferna6
 * Course Number: ECE 2220
 * Semester: Spring 2025
 * Assignment Number: Lab 6 – Structures - Bitmap
 *
 * Purpose:
 *   This program reads and manipulates 24-bit BMP images as outlined in
 *   the project statement. It supports three operations:
 *      1) read  : Read a .bmp file and output the header information
 *                 and pixel data (including any padding) to a text file.
 *      2) edge  : Apply an edge-detection transform and write out a new
 *                 .bmp image named "<original>-edge.bmp".
 *      3) noise : Apply additive Gaussian noise and write out a new .bmp
 *                 image named "<original>-noise.bmp".
 *
 * Assumptions:
 *   - The input .bmp file is a 24-bit color image (no compression).
 *   - For "read", the program is invoked with: p6 read <input.bmp> <output.txt>
 *   - For "edge" or "noise", the program is invoked with: p6 edge <input.bmp>
 *     or p6 noise <input.bmp>.
 *   - The user is prompted for standard deviation (5 to 20) when performing the
 *     noise operation.
 *   - Header fields are read field-by-field to avoid structure alignment issues.
 *
 * Known Bugs:
 *   - The edge-detection or noise operations may need extra checks for
 *     boundary pixels or for clamping pixel values to [0..255].
 *   - The “read” operation prints header bytes and pixels in a simplified
 *     manner; additional formatting or printing might be needed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Structure to represent the first BMP header (14 bytes)
struct Header {
    unsigned short type;       // Magic identifier (should be "BM")
    unsigned int size;         // Size of the file in bytes
    unsigned short reserved1;  // Reserved, usually 0
    unsigned short reserved2;  // Reserved, usually 0
    unsigned int offset;       // Offset to the start of pixel data (in bytes)
};

// Structure to represent the BMP info header (40 bytes)
struct InfoHeader {
    unsigned int size;         // Header size in bytes
    int width;                 // Image width in pixels
    int height;                // Image height in pixels
    unsigned short planes;     // Number of color planes (should be 1)
    unsigned short bits;       // Bits per pixel (should be 24 for our images)
    unsigned int compression;  // Compression type (0 for uncompressed)
    unsigned int imageSize;    // Size of the image data in bytes
    int xResolution;           // Pixels per meter in X-direction
    int yResolution;           // Pixels per meter in Y-direction
    unsigned int colors;       // Number of colors in the palette
    unsigned int importantColors; // Number of important colors
};

// Structure to represent a pixel with RGB components.
struct PIXEL {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

// Function prototypes
int readOperation(const char *inputFile, const char *outputFile);
int edgeOperation(const char *inputFile);
int noiseOperation(const char *inputFile);
double generateGaussian(double mean, double stddev);
unsigned char clamp(int value);

//
// main - Parses command line arguments and calls the appropriate operation
//
int main(int argc, char *argv[])
{
    // Seed the random number generator (used for noise generation)
    srand((unsigned)time(NULL));

    // Check for at least 3 arguments (operation and input file).
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <operation> <input file> [output file]\n", argv[0]);
        exit(1);
    }

    // Determine which operation to perform (read, edge, or noise)
    if (strcmp(argv[1], "read") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage for read: %s read <input.bmp> <output.txt>\n", argv[0]);
            exit(1);
        }
        return readOperation(argv[2], argv[3]);
    }
    else if (strcmp(argv[1], "edge") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage for edge: %s edge <input.bmp>\n", argv[0]);
            exit(1);
        }
        return edgeOperation(argv[2]);
    }
    else if (strcmp(argv[1], "noise") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage for noise: %s noise <input.bmp>\n", argv[0]);
            exit(1);
        }
        return noiseOperation(argv[2]);
    }
    else {
        fprintf(stderr, "Invalid operation: %s\n", argv[1]);
        exit(1);
    }
    return 0;
}

//
// readOperation - Performs the "read" operation by reading the BMP header
// and pixel data then printing the details into an output text file.
//
int readOperation(const char *inputFile, const char *outputFile)
{
    // Open the BMP input file in binary mode.
    FILE *fp = fopen(inputFile, "rb");
    if (!fp) {
        perror("Error opening input file");
        return 1;
    }

    // Open the output file in text mode.
    FILE *out = fopen(outputFile, "w");
    if (!out) {
        perror("Error opening output file");
        fclose(fp);
        return 1;
    }

    // Read the 54-byte BMP header into a buffer.
    unsigned char headerBuffer[54];
    if (fread(headerBuffer, 1, 54, fp) != 54) {
        fprintf(stderr, "Error reading BMP header.\n");
        fclose(fp);
        fclose(out);
        return 1;
    }

    // Decode the BMP Header fields from the buffer (little endian)
    struct Header header;
    header.type = headerBuffer[0] | (headerBuffer[1] << 8);
    header.size = headerBuffer[2] | (headerBuffer[3] << 8) |
                  (headerBuffer[4] << 16) | (headerBuffer[5] << 24);
    header.reserved1 = headerBuffer[6] | (headerBuffer[7] << 8);
    header.reserved2 = headerBuffer[8] | (headerBuffer[9] << 8);
    header.offset = headerBuffer[10] | (headerBuffer[11] << 8) |
                    (headerBuffer[12] << 16) | (headerBuffer[13] << 24);

    // Decode the BMP InfoHeader fields
    struct InfoHeader info;
    info.size = headerBuffer[14] | (headerBuffer[15] << 8) |
                (headerBuffer[16] << 16) | (headerBuffer[17] << 24);
    info.width = headerBuffer[18] | (headerBuffer[19] << 8) |
                 (headerBuffer[20] << 16) | (headerBuffer[21] << 24);
    info.height = headerBuffer[22] | (headerBuffer[23] << 8) |
                  (headerBuffer[24] << 16) | (headerBuffer[25] << 24);
    info.planes = headerBuffer[26] | (headerBuffer[27] << 8);
    info.bits = headerBuffer[28] | (headerBuffer[29] << 8);
    info.compression = headerBuffer[30] | (headerBuffer[31] << 8) |
                       (headerBuffer[32] << 16) | (headerBuffer[33] << 24);
    info.imageSize = headerBuffer[34] | (headerBuffer[35] << 8) |
                     (headerBuffer[36] << 16) | (headerBuffer[37] << 24);
    info.xResolution = headerBuffer[38] | (headerBuffer[39] << 8) |
                       (headerBuffer[40] << 16) | (headerBuffer[41] << 24);
    info.yResolution = headerBuffer[42] | (headerBuffer[43] << 8) |
                       (headerBuffer[44] << 16) | (headerBuffer[45] << 24);
    info.colors = headerBuffer[46] | (headerBuffer[47] << 8) |
                  (headerBuffer[48] << 16) | (headerBuffer[49] << 24);
    info.importantColors = headerBuffer[50] | (headerBuffer[51] << 8) |
                           (headerBuffer[52] << 16) | (headerBuffer[53] << 24);

    // Calculate the number of padded bytes per row.
    int rowSize = info.width * 3; // each pixel is 3 bytes (RGB)
    int padding = (4 - (rowSize % 4)) % 4;

    // Print the file name and header data to the output file.
    fprintf(out, "\"%s\"\n", inputFile);
    // Print the two individual header type bytes (usually 'B' and 'M')
    fprintf(out, "Header.Type = %c\n", headerBuffer[0]);
    fprintf(out, "Header.Type = %c\n", headerBuffer[1]);
    fprintf(out, "Header.Size = %u\n", header.size);
    fprintf(out, "Header.Offset = %u\n", header.offset);
    fprintf(out, "InfoHeader.Size = %u\n", info.size);
    fprintf(out, "InfoHeader.Width = %d\n", info.width);
    fprintf(out, "InfoHeader.Height = %d\n", info.height);
    fprintf(out, "InfoHeader.Planes = %u\n", info.planes);
    fprintf(out, "InfoHeader.Bits = %u\n", info.bits);
    fprintf(out, "InfoHeader.Compression = %u\n", info.compression);
    fprintf(out, "InfoHeader.ImageSize = %u\n", info.imageSize);
    fprintf(out, "InfoHeader.xResolution = %d\n", info.xResolution);
    fprintf(out, "InfoHeader.yResolution = %d\n", info.yResolution);
    fprintf(out, "InfoHeader.Colors = %u\n", info.colors);
    fprintf(out, "InfoHeader.ImportantColors = %u\n", info.importantColors);
    fprintf(out, "Padding = %d\n", padding);

    // Print out each individual byte of the header.
    for (int i = 0; i < 54; i++) {
        fprintf(out, "Byte[%d] = %03d\n", i, headerBuffer[i]);
    }

    // Read and print the pixel data
    // Since BMP pixels are stored as rows (with potential padding at the end),
    // we read one row at a time.
    int width = info.width;
    int height = info.height;
    for (int i = 0; i < height; i++) {
        // For each pixel in the current row
        for (int j = 0; j < width; j++) {
            // Each pixel has 3 bytes, stored in BMP as B, G, R.
            unsigned char color[3];
            if (fread(color, 1, 3, fp) != 3) {
                fprintf(stderr, "Error reading pixel data.\n");
                fclose(fp);
                fclose(out);
                return 1;
            }
            // Rearrange into RGB order for printing.
            fprintf(out, "RGB[%d,%d] = %03d.%03d.%03d\n", i, j, color[2], color[1], color[0]);
        }
        // For any padding bytes at the end of the row, read and print them.
        for (int k = 0; k < padding; k++) {
            unsigned char padByte;
            if (fread(&padByte, 1, 1, fp) != 1) {
                fprintf(stderr, "Error reading padding data.\n");
                fclose(fp);
                fclose(out);
                return 1;
            }
            fprintf(out, "Padding[%d] = %03d\n", k, padByte);
        }
    }

    fclose(fp);
    fclose(out);
    return 0;
}

//
// edgeOperation - Performs the "edge" operation by reading the BMP image,
// applying an edge-detection filter, and writing a new BMP file with "-edge" inserted
// in the original filename.
//
int edgeOperation(const char *inputFile)
{
    // Open the BMP input file in binary mode.
    FILE *fp = fopen(inputFile, "rb");
    if (!fp) {
        perror("Error opening input file");
        return 1;
    }

    // Read the 54-byte BMP header.
    unsigned char headerBuffer[54];
    if (fread(headerBuffer, 1, 54, fp) != 54) {
        fprintf(stderr, "Error reading BMP header.\n");
        fclose(fp);
        return 1;
    }

    // Decode the InfoHeader fields from the header (bytes 14-53).
    struct InfoHeader info;
    info.size = headerBuffer[14] | (headerBuffer[15] << 8) |
                (headerBuffer[16] << 16) | (headerBuffer[17] << 24);
    info.width = headerBuffer[18] | (headerBuffer[19] << 8) |
                 (headerBuffer[20] << 16) | (headerBuffer[21] << 24);
    info.height = headerBuffer[22] | (headerBuffer[23] << 8) |
                  (headerBuffer[24] << 16) | (headerBuffer[25] << 24);
    info.planes = headerBuffer[26] | (headerBuffer[27] << 8);
    info.bits = headerBuffer[28] | (headerBuffer[29] << 8);
    info.compression = headerBuffer[30] | (headerBuffer[31] << 8) |
                       (headerBuffer[32] << 16) | (headerBuffer[33] << 24);
    info.imageSize = headerBuffer[34] | (headerBuffer[35] << 8) |
                     (headerBuffer[36] << 16) | (headerBuffer[37] << 24);
    info.xResolution = headerBuffer[38] | (headerBuffer[39] << 8) |
                       (headerBuffer[40] << 16) | (headerBuffer[41] << 24);
    info.yResolution = headerBuffer[42] | (headerBuffer[43] << 8) |
                       (headerBuffer[44] << 16) | (headerBuffer[45] << 24);
    info.colors = headerBuffer[46] | (headerBuffer[47] << 8) |
                  (headerBuffer[48] << 16) | (headerBuffer[49] << 24);
    info.importantColors = headerBuffer[50] | (headerBuffer[51] << 8) |
                           (headerBuffer[52] << 16) | (headerBuffer[53] << 24);

    int width = info.width;
    int height = info.height;
    int rowSize = width * 3;
    int padding = (4 - (rowSize % 4)) % 4;

    // Dynamically allocate a 2D array to hold the pixel data.
    struct PIXEL **pixels = malloc(height * sizeof(struct PIXEL *));
    if (pixels == NULL) {
        perror("Memory allocation error");
        fclose(fp);
        return 1;
    }
    for (int i = 0; i < height; i++) {
        pixels[i] = malloc(width * sizeof(struct PIXEL));
        if (pixels[i] == NULL) {
            perror("Memory allocation error");
            fclose(fp);
            return 1;
        }
    }

    // Read the pixel data from the file.
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            unsigned char color[3];
            if (fread(color, 1, 3, fp) != 3) {
                fprintf(stderr, "Error reading pixel data.\n");
                fclose(fp);
                return 1;
            }
            // BMP format stores pixels in Blue, Green, Red order.
            pixels[i][j].blue = color[0];
            pixels[i][j].green = color[1];
            pixels[i][j].red = color[2];
        }
        // Skip over any padding bytes.
        fseek(fp, padding, SEEK_CUR);
    }
    fclose(fp);

    // Allocate a new 2D array for the edge-detected pixel data.
    struct PIXEL **edgePixels = malloc(height * sizeof(struct PIXEL *));
    if (edgePixels == NULL) {
        perror("Memory allocation error");
        return 1;
    }
    for (int i = 0; i < height; i++) {
        edgePixels[i] = malloc(width * sizeof(struct PIXEL));
        if (edgePixels[i] == NULL) {
            perror("Memory allocation error");
            return 1;
        }
    }

    // Define the 3x3 edge detection filter matrix.
    int matrix[3][3] = { {  0, -1,  0 },
                         { -1,  4, -1 },
                         {  0, -1,  0 } };

    // Apply the convolution filter to each pixel.
    // For boundary pixels, simply copy the original value.
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (i == 0 || i == height - 1 || j == 0 || j == width - 1) {
                edgePixels[i][j] = pixels[i][j];
            }
            else {
                int sumR = 0, sumG = 0, sumB = 0;
                // Overlay the 3x3 filter matrix on the current pixel.
                for (int m = -1; m <= 1; m++) {
                    for (int n = -1; n <= 1; n++) {
                        int factor = matrix[m+1][n+1];
                        sumR += factor * pixels[i + m][j + n].red;
                        sumG += factor * pixels[i + m][j + n].green;
                        sumB += factor * pixels[i + m][j + n].blue;
                    }
                }
                // Clamp the results to the valid range [0, 255].
                edgePixels[i][j].red   = clamp(sumR);
                edgePixels[i][j].green = clamp(sumG);
                edgePixels[i][j].blue  = clamp(sumB);
            }
        }
    }

    // Create the output filename by inserting "-edge" before the ".bmp" extension.
    char outFilename[256];
    strcpy(outFilename, inputFile);
    char *dot = strrchr(outFilename, '.');
    if (dot != NULL) {
        *dot = '\0';
        strcat(outFilename, "-edge.bmp");
    } else {
        strcat(outFilename, "-edge.bmp");
    }

    // Open the output file in binary mode.
    fp = fopen(outFilename, "wb");
    if (!fp) {
        perror("Error creating output file");
        return 1;
    }

    // Write the original BMP header.
    fwrite(headerBuffer, 1, 54, fp);

    // Write the new (edge-detected) pixel data, including proper padding.
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // BMP files store pixel data in Blue, Green, Red order.
            unsigned char color[3];
            color[0] = edgePixels[i][j].blue;
            color[1] = edgePixels[i][j].green;
            color[2] = edgePixels[i][j].red;
            fwrite(color, 1, 3, fp);
        }
        // Write the necessary padding bytes (set to 0).
        unsigned char pad = 0;
        for (int k = 0; k < padding; k++) {
            fwrite(&pad, 1, 1, fp);
        }
    }
    fclose(fp);

    // Free dynamically allocated memory.
    for (int i = 0; i < height; i++) {
        free(pixels[i]);
        free(edgePixels[i]);
    }
    free(pixels);
    free(edgePixels);

    return 0;
}

//
// noiseOperation - Performs the "noise" operation by reading the BMP image,
// adding Gaussian (Box-Muller) noise to each pixel, and writing a new BMP file with "-noise"
// inserted in the original filename.
//
int noiseOperation(const char *inputFile)
{
    // Open the BMP input file in binary mode.
    FILE *fp = fopen(inputFile, "rb");
    if (!fp) {
        perror("Error opening input file");
        return 1;
    }

    // Read the 54-byte BMP header.
    unsigned char headerBuffer[54];
    if (fread(headerBuffer, 1, 54, fp) != 54) {
        fprintf(stderr, "Error reading BMP header.\n");
        fclose(fp);
        return 1;
    }

    // Decode the InfoHeader fields from the header.
    struct InfoHeader info;
    info.size = headerBuffer[14] | (headerBuffer[15] << 8) |
                (headerBuffer[16] << 16) | (headerBuffer[17] << 24);
    info.width = headerBuffer[18] | (headerBuffer[19] << 8) |
                 (headerBuffer[20] << 16) | (headerBuffer[21] << 24);
    info.height = headerBuffer[22] | (headerBuffer[23] << 8) |
                  (headerBuffer[24] << 16) | (headerBuffer[25] << 24);
    info.planes = headerBuffer[26] | (headerBuffer[27] << 8);
    info.bits = headerBuffer[28] | (headerBuffer[29] << 8);
    info.compression = headerBuffer[30] | (headerBuffer[31] << 8) |
                       (headerBuffer[32] << 16) | (headerBuffer[33] << 24);
    info.imageSize = headerBuffer[34] | (headerBuffer[35] << 8) |
                     (headerBuffer[36] << 16) | (headerBuffer[37] << 24);
    info.xResolution = headerBuffer[38] | (headerBuffer[39] << 8) |
                       (headerBuffer[40] << 16) | (headerBuffer[41] << 24);
    info.yResolution = headerBuffer[42] | (headerBuffer[43] << 8) |
                       (headerBuffer[44] << 16) | (headerBuffer[45] << 24);
    info.colors = headerBuffer[46] | (headerBuffer[47] << 8) |
                  (headerBuffer[48] << 16) | (headerBuffer[49] << 24);
    info.importantColors = headerBuffer[50] | (headerBuffer[51] << 8) |
                           (headerBuffer[52] << 16) | (headerBuffer[53] << 24);

    int width = info.width;
    int height = info.height;
    int rowSize = width * 3;
    int padding = (4 - (rowSize % 4)) % 4;

    // Dynamically allocate a 2D array to hold the pixel data.
    struct PIXEL **pixels = malloc(height * sizeof(struct PIXEL *));
    if (pixels == NULL) {
        perror("Memory allocation error");
        fclose(fp);
        return 1;
    }
    for (int i = 0; i < height; i++) {
        pixels[i] = malloc(width * sizeof(struct PIXEL));
        if (pixels[i] == NULL) {
            perror("Memory allocation error");
            fclose(fp);
            return 1;
        }
    }

    // Read the pixel data.
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            unsigned char color[3];
            if (fread(color, 1, 3, fp) != 3) {
                fprintf(stderr, "Error reading pixel data.\n");
                fclose(fp);
                return 1;
            }
            // Convert from BMP (B, G, R) to our PIXEL structure (R, G, B).
            pixels[i][j].blue = color[0];
            pixels[i][j].green = color[1];
            pixels[i][j].red = color[2];
        }
        fseek(fp, padding, SEEK_CUR);
    }
    fclose(fp);

    // Prompt the user for the standard deviation (from 5 to 20) for the Gaussian noise.
    double stddev;
    printf("Enter standard deviation for noise (5 to 20): ");
    scanf("%lf", &stddev);
    if (stddev < 5 || stddev > 20) {
        printf("Standard deviation out of range. Setting to 5.\n");
        stddev = 5;
    }

    // Add Gaussian noise to each pixel using the Box-Muller transform.
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // Generate noise for each color and add it to the original value.
            int newRed   = pixels[i][j].red   + (int)round(generateGaussian(0, stddev));
            int newGreen = pixels[i][j].green + (int)round(generateGaussian(0, stddev));
            int newBlue  = pixels[i][j].blue  + (int)round(generateGaussian(0, stddev));

            // Clamp the values to the valid range.
            pixels[i][j].red   = clamp(newRed);
            pixels[i][j].green = clamp(newGreen);
            pixels[i][j].blue  = clamp(newBlue);
        }
    }

    // Create the output filename by inserting "-noise" before ".bmp"
    char outFilename[256];
    strcpy(outFilename, inputFile);
    char *dot = strrchr(outFilename, '.');
    if (dot != NULL) {
        *dot = '\0';
        strcat(outFilename, "-noise.bmp");
    }
    else {
        strcat(outFilename, "-noise.bmp");
    }

    // Open the output file in binary mode.
    fp = fopen(outFilename, "wb");
    if (!fp) {
        perror("Error creating output file");
        return 1;
    }

    // Write the header to the output file.
    fwrite(headerBuffer, 1, 54, fp);

    // Write the noisy pixel data with the appropriate row padding.
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // Write pixel in BMP order: Blue, Green, Red.
            unsigned char color[3];
            color[0] = pixels[i][j].blue;
            color[1] = pixels[i][j].green;
            color[2] = pixels[i][j].red;
            fwrite(color, 1, 3, fp);
        }
        // Write padding bytes (0s).
        unsigned char pad = 0;
        for (int k = 0; k < padding; k++) {
            fwrite(&pad, 1, 1, fp);
        }
    }
    fclose(fp);

    // Free allocated memory.
    for (int i = 0; i < height; i++) {
        free(pixels[i]);
    }
    free(pixels);

    return 0;
}

//
// generateGaussian - Uses the Box-Muller transform to generate a Gaussian-
// distributed random number with the given mean and standard deviation.
//
double generateGaussian(double mean, double stddev)
{
    // Generate two uniformly distributed random numbers in (0,1)
    double u1 = (rand() + 1.0) / (RAND_MAX + 1.0);
    double u2 = (rand() + 1.0) / (RAND_MAX + 1.0);
    // Apply Box-Muller transform
    double z0 = sqrt(-2.0 * log(u1)) * cos(2 * M_PI * u2);
    return z0 * stddev + mean;
}

//
// clamp - Clamps an integer value to the range [0, 255] and returns it as an unsigned char.
//
unsigned char clamp(int value)
{
    if (value < 0)
        return 0;
    if (value > 255)
        return 255;
    return (unsigned char)value;
}

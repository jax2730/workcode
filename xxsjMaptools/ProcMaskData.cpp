#include "FreeImage/FreeImage.h"
#include "KarstDataHelper.h"
#include <cstdio>
#include <memory>
#include <string>

void ProcessMaskTiledTiff(const std::string& inputDir, const std::string& outputPath, bool* selectedGrid, int tileRows, int tileCols, const std::string& tilePattern = "tile_%03d_%03d.tif")
{
	// 创建输出文件
	FILE* outputFile = fopen(outputPath.c_str(), "wb");
	if (!outputFile) {
		printf("Failed to create output file: %s\n", outputPath.c_str());
		return;
	}
	
	// 写入瓦片总数信息
	int totalTiles = tileRows * tileCols;
	fwrite(&totalTiles, sizeof(int), 1, outputFile);
	fwrite(&tileRows, sizeof(int), 1, outputFile);
	fwrite(&tileCols, sizeof(int), 1, outputFile);
	
	int processedTiles = 0;
	
	for (int row = 0; row < tileRows; row++) {
		for (int col = 0; col < tileCols; col++) {

			if (!selectedGrid[col * tileRows + row]) {
				printf("Warning: Grid is not selected: %d, %d\n", row, col);
				continue;
			}

			char tileFilePath[512];
			sprintf_s(tileFilePath, sizeof(tileFilePath), (inputDir + "/" + tilePattern).c_str(), col + 1, row + 1);
			
			FIBITMAP* tileBitmap = FreeImage_Load(FIF_TIFF, tileFilePath);
			if (!tileBitmap) {
				printf("Warning: Failed to load tile: %s\n", tileFilePath);
				continue;
			}
			
			int tileWidth = FreeImage_GetWidth(tileBitmap);
			int tileHeight = FreeImage_GetHeight(tileBitmap);
			
			std::unique_ptr<BYTE[]> tileBuffer(new BYTE[tileWidth * tileHeight]);
			
			bool invalid = true;
			for (int h = tileHeight - 1; h >= 0; h--) {
				BYTE* scanline = FreeImage_GetScanLine(tileBitmap, h);
				for (int w = 0; w < tileWidth; w++) {
					int bufferIndex = (tileHeight - 1 - h) * tileWidth + w;
					tileBuffer[bufferIndex] = scanline[w];
					
					if (scanline[w] != 127 && scanline[w] != 0) {
						invalid = false;
					}
				}
			}
			
			FreeImage_Unload(tileBitmap);
			
			// 如果所有像素值都无效，跳过保存该瓦片
			if (invalid) {
				printf("Skipping tile %d (%03d,%03d): all values are invalidate\n", 
					   row * tileCols + col, row + 1, col + 1);
				continue;
			}
			
			// 写入瓦片头部信息
			int tileNumber = row * tileCols + col;  // 瓦片编号
			fwrite(&tileNumber, sizeof(int), 1, outputFile);  // 瓦片编号
			fwrite(&row, sizeof(int), 1, outputFile);         // 瓦片行号
			fwrite(&col, sizeof(int), 1, outputFile);         // 瓦片列号
			fwrite(&tileWidth, sizeof(int), 1, outputFile);   // 瓦片宽度
			fwrite(&tileHeight, sizeof(int), 1, outputFile);  // 瓦片高度
			
			// 写入瓦片数据
			size_t written = fwrite(tileBuffer.get(), sizeof(BYTE), tileWidth * tileHeight, outputFile);
			if (written != tileWidth * tileHeight) {
				printf("Warning: Failed to write complete tile data for tile %d\n", tileNumber);
			}
			
			processedTiles++;
			
			printf("Processed tile %d (%03d,%03d): %dx%d\n", tileNumber, row + 1, col + 1, tileWidth, tileHeight);
		}
	}
	
	fclose(outputFile);
	printf("Successfully processed %d tiles into %s\n", processedTiles, outputPath.c_str());
}

void ProcessAreaIDTiledTiff(const std::string& inputDir, const std::string& outputPath, bool* selectedGrid, int tileRows, int tileCols, const std::string& tilePattern = "tile_%d_%d.tif")
{
	// 创建输出文件
	FILE* outputFile = fopen(outputPath.c_str(), "wb");
	if (!outputFile) {
		printf("Failed to create output file: %s\n", outputPath.c_str());
		return;
	}
	
	// 写入瓦片总数信息
	int totalTiles = tileRows * tileCols;
	fwrite(&totalTiles, sizeof(int), 1, outputFile);
	fwrite(&tileRows, sizeof(int), 1, outputFile);
	fwrite(&tileCols, sizeof(int), 1, outputFile);
	
	int processedTiles = 0;
	
	for (int row = 0; row < tileRows; row++) {
		for (int col = 0; col < tileCols; col++) {

			if (!selectedGrid[col * tileRows + row]) {
				printf("Warning: Grid is not selected: %d, %d\n", row, col);
				continue;
			}

			char tileFilePath[512];
			sprintf_s(tileFilePath, sizeof(tileFilePath), (inputDir + "/" + tilePattern).c_str(), col + 1, row + 1);
			
			FIBITMAP* tileBitmap = FreeImage_Load(FIF_TIFF, tileFilePath);
			if (!tileBitmap) {
				printf("Warning: Failed to load tile: %s\n", tileFilePath);
				continue;
			}
			
			int tileWidth = FreeImage_GetWidth(tileBitmap);
			int tileHeight = FreeImage_GetHeight(tileBitmap);
			
			// 使用 uint32_t 缓冲区而不是 BYTE
			std::unique_ptr<uint32_t[]> tileBuffer(new uint32_t[tileWidth * tileHeight]);
			
			bool invalid = true;
			for (int h = tileHeight - 1; h >= 0; h--) {
				uint32_t* scanline = (uint32_t*)FreeImage_GetScanLine(tileBitmap, h);
				for (int w = 0; w < tileWidth; w++) {
					int bufferIndex = (tileHeight - 1 - h) * tileWidth + w;
					tileBuffer[bufferIndex] = scanline[w];
					
					if (scanline[w] != 0xffffffff) {
						invalid = false;
					}
				}
			}
			
			FreeImage_Unload(tileBitmap);
			
			// 如果所有像素值都无效，跳过保存该瓦片
			if (invalid) {
				printf("Skipping tile %d (%03d,%03d): all area ID values are invalid\n", 
					   row * tileCols + col, row + 1, col + 1);
				continue;
			}
			
			// 写入瓦片头部信息
			int tileNumber = row * tileCols + col;  // 瓦片编号
			fwrite(&tileNumber, sizeof(int), 1, outputFile);  // 瓦片编号
			fwrite(&row, sizeof(int), 1, outputFile);         // 瓦片行号
			fwrite(&col, sizeof(int), 1, outputFile);         // 瓦片列号
			fwrite(&tileWidth, sizeof(int), 1, outputFile);   // 瓦片宽度
			fwrite(&tileHeight, sizeof(int), 1, outputFile);  // 瓦片高度
			
			// 写入瓦片数据 (uint32_t 数据)
			size_t written = fwrite(tileBuffer.get(), sizeof(uint32_t), tileWidth * tileHeight, outputFile);
			if (written != tileWidth * tileHeight) {
				printf("Warning: Failed to write complete tile data for tile %d\n", tileNumber);
			}
			
			processedTiles++;
			
			printf("Processed AreaID tile %d (%03d,%03d): %dx%d\n", tileNumber, row + 1, col + 1, tileWidth, tileHeight);
		}
	}
	
	fclose(outputFile);
	printf("Successfully processed %d AreaID tiles into %s\n", processedTiles, outputPath.c_str());
}

void readTiledData(const std::string& inputPath)
{
	FILE* inputFile = fopen(inputPath.c_str(), "rb");
	if (!inputFile) {
		printf("Failed to open input file: %s\n", inputPath.c_str());
		return;
	}
	
	// 读取文件头部
	int totalTiles, tileRows, tileCols;
	fread(&totalTiles, sizeof(int), 1, inputFile);
	fread(&tileRows, sizeof(int), 1, inputFile);
	fread(&tileCols, sizeof(int), 1, inputFile);
	
	printf("File contains %d tiles (%dx%d grid)\n", totalTiles, tileRows, tileCols);
	
	// 读取每个瓦片
	for (int i = 0; i < totalTiles; i++) {
		// 读取瓦片头部
		int tileNumber, row, col, width, height;
		fread(&tileNumber, sizeof(int), 1, inputFile);
		fread(&row, sizeof(int), 1, inputFile);
		fread(&col, sizeof(int), 1, inputFile);
		fread(&width, sizeof(int), 1, inputFile);
		fread(&height, sizeof(int), 1, inputFile);
		
		// 读取瓦片数据
		size_t dataSize = width * height;
		std::unique_ptr<BYTE[]> tileData(new BYTE[dataSize]);
		fread(tileData.get(), sizeof(BYTE), dataSize, inputFile);
		
		printf("Tile %d: position(%d,%d), size(%dx%d), data size: %zu bytes\n", 
			   tileNumber, row, col, width, height, dataSize);
	}
	
	fclose(inputFile);
}

void readAreaIDTiledData(const std::string& inputPath)
{
	FILE* inputFile = fopen(inputPath.c_str(), "rb");
	if (!inputFile) {
		printf("Failed to open input file: %s\n", inputPath.c_str());
		return;
	}
	
	// 读取文件头部
	int totalTiles, tileRows, tileCols;
	fread(&totalTiles, sizeof(int), 1, inputFile);
	fread(&tileRows, sizeof(int), 1, inputFile);
	fread(&tileCols, sizeof(int), 1, inputFile);
	
	printf("AreaID file contains %d tiles (%dx%d grid)\n", totalTiles, tileRows, tileCols);
	
	// 读取每个瓦片
	for (int i = 0; i < totalTiles; i++) {
		// 读取瓦片头部
		int tileNumber, row, col, width, height;
		fread(&tileNumber, sizeof(int), 1, inputFile);
		fread(&row, sizeof(int), 1, inputFile);
		fread(&col, sizeof(int), 1, inputFile);
		fread(&width, sizeof(int), 1, inputFile);
		fread(&height, sizeof(int), 1, inputFile);
		
		// 读取瓦片数据 (uint32_t 数据)
		size_t dataSize = width * height;
		std::unique_ptr<uint32_t[]> tileData(new uint32_t[dataSize]);
		fread(tileData.get(), sizeof(uint32_t), dataSize, inputFile);
		
		printf("AreaID Tile %d: position(%d,%d), size(%dx%d), data size: %zu uint32_t values (%zu bytes)\n", 
			   tileNumber, row, col, width, height, dataSize, dataSize * sizeof(uint32_t));
		
		uint32_t minValue = UINT32_MAX;
		uint32_t maxValue = 0;
		int validPixels = 0;
		
		for (size_t j = 0; j < dataSize; j++) {
			if (tileData[j] != 0xffffffff) {
				validPixels++;
				if (tileData[j] < minValue) minValue = tileData[j];
				if (tileData[j] > maxValue) maxValue = tileData[j];
			}
		}
		
		if (validPixels > 0) {
			printf("  Valid pixels: %d, Area ID range: %u - %u\n", validPixels, minValue, maxValue);
		} else {
			printf("  No valid Area ID values found\n");
		}
	}
	
	fclose(inputFile);
}

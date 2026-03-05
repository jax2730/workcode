#include <limits>
#include <memory>
#include <map>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <Vector>
#include <stdio.h>
#include <algorithm>
#include <string>
#include "FreeImage/FreeImage.h"

struct Vector3 {
	float x, y, z;
	inline Vector3() :x(0), y(0), z(0) {}
	inline Vector3(const float fX, const float fY, const float fZ) : x(fX), y(fY), z(fZ) {}
	inline Vector3 crossProduct(const Vector3& rkVector) const {
		return Vector3(
			y * rkVector.z - z * rkVector.y,
			z * rkVector.x - x * rkVector.z,
			x * rkVector.y - y * rkVector.x);
	}
	inline float normalise() {
		float fLength = (float)sqrt(x * x + y * y + z * z);

		// Will also work for zero-sized vectors, but will change nothing
		// We're not using epsilons because we don't need to.
		// Read http://www.ogre3d.org/forums/viewtopic.php?f=4&t=61259
		if (fLength > float(0.0f)) {
			float fInvLength = 1.0f / fLength;
			x *= fInvLength;
			y *= fInvLength;
			z *= fInvLength;
		}

		return fLength;
	}
	inline Vector3 operator + (const Vector3& rkVector) const {
		return Vector3(
			x + rkVector.x,
			y + rkVector.y,
			z + rkVector.z);
	}
};

const unsigned int HeightMapFileMagicID = 'T' | ('E' << 8) | ('H' << 16) | ('M' << 24);
const unsigned int TexutreFileMagicID = 'T' | ('E' << 8) | ('T' << 16) | ('X' << 24);

const int HeightMapPixPerPage = 1024;
const int TexturePixPerPage = 1024;
//const int TargetPixPerPage = 256;

const int hAll = 1690;
const int hMin = -625;

inline uint32_t ConvertHeightFloatToU16(float height) {
	return uint32_t(std::max((height - hMin), 0.0f) / hAll * 65535);
}

inline uint32_t ConvertSlopeFloatToU8(float height) {
	height = std::min(1.0f, std::max(height, 0.0f));
	return uint32_t(height * 255);
}

#define GETHEIGHT(x) float(x >> 16) / 65535.0f * hAll + hMin
#define GETSLOPE(x) float((x >> 8) & 0xff) / 255.0f

bool CheckFileAvalible(const std::vector<float>& tmpbuf) {
	bool avalible = false;
	for (const float& val : tmpbuf) {
		if (val > 0.0) {
			avalible = true;
			break;
		}
	}
	return avalible;
}

void WriteHeightDataToFile(uint32_t* heightData, int srcSize, int targetSize, FILE* file) {
	int scaleLevel = srcSize / targetSize;
	fwrite(&HeightMapFileMagicID, sizeof(HeightMapFileMagicID), 1, file);
	fwrite(&srcSize, sizeof(srcSize), 1, file);
	fwrite(&targetSize, sizeof(targetSize), 1, file);

	if (srcSize == targetSize) {
		int count = (srcSize + 3) * (srcSize + 3);
		fwrite(heightData, sizeof(uint32_t), count, file);
		return;
	}
	srcSize += 3;
	//write top
	fwrite(heightData, sizeof(uint32_t), srcSize, file);
	fwrite(heightData + srcSize, sizeof(uint32_t), srcSize, file);
	//write bottom
	fwrite(heightData + srcSize * (srcSize - 2), sizeof(uint32_t), srcSize, file);
	fwrite(heightData + srcSize * (srcSize - 1), sizeof(uint32_t), srcSize, file);
	//write left
	for (int i = 0; i < srcSize; i++) {
		fwrite(heightData + i * srcSize, sizeof(uint32_t), 1, file);
	}
	for (int i = 0; i < srcSize; i++) {
		fwrite(heightData + i * srcSize + 1, sizeof(uint32_t), 1, file);
	}
	//write right
	for (int i = 0; i < srcSize; i++) {
		fwrite(heightData + (i + 1) * srcSize - 2, sizeof(uint32_t), 1, file);
	}
	for (int i = 0; i < srcSize; i++) {
		fwrite(heightData + (i + 1) * srcSize - 1, sizeof(uint32_t), 1, file);
	}
	int rowSize = targetSize + 1;
	std::vector<uint32_t> hmTmpData(rowSize * rowSize);
	for (int y = 0; y < rowSize; y++) {
		for (int x = 0; x < rowSize; x++) {
			int idx = y * rowSize + x;
			hmTmpData[idx] = heightData[(scaleLevel * y + 1) * srcSize + (scaleLevel * x + 1)];
		}
	}
	fwrite(hmTmpData.data(), sizeof(uint32_t), rowSize * rowSize, file);
}

void ConvertWorldHeightMapData(const std::string& originalPath, const std::string& resPath, bool highResolution, float KarstSize = 32768.0f, int targetPixPerPage = 256) {
	FIBITMAP* fibitmap = FreeImage_Load(FIF_TIFF, originalPath.c_str());

	int width = FreeImage_GetWidth(fibitmap);
	int height = FreeImage_GetHeight(fibitmap);

	std::vector<float> tmpbuf(width * height);
	int count = 0;
	for (int h = height - 1; h >= 0; --h, ++count) {
		BYTE* scanline = FreeImage_GetScanLine(fibitmap, h);
		float* _f32 = reinterpret_cast<float*>(scanline);
		for (int w = 0; w < width; ++w) {
			tmpbuf[count * width + w] = _f32[w];
		}
	}

	if (!CheckFileAvalible(tmpbuf)) {
		FreeImage_Unload(fibitmap);
		return;
	}

	int HMPixVertexPerPage = (HeightMapPixPerPage + 3);
	int vertCount = HMPixVertexPerPage * HMPixVertexPerPage;

	auto clampIdx_X = [width](int num) {
		return std::max(std::min(num, (int)width - 1), 0);
		};

	auto clampIdx_Y = [height](int num) {
		return std::max(std::min(num, (int)height - 1), 0);
		};

	FILE* file_hm;
	fopen_s(&file_hm, resPath.c_str(), "wb");

	std::vector<uint32_t> hmTmpData(HMPixVertexPerPage * HMPixVertexPerPage);

	float delta = KarstSize / HeightMapPixPerPage;

	for (int y = 0; y < HMPixVertexPerPage; y++) {
		for (int x = 0; x < HMPixVertexPerPage; x++) {
			int ix = x;
			int iy = y;
			int idx = y * HMPixVertexPerPage + x;
			float value = tmpbuf[iy * width + ix] / 8.f;
			value = isnan(value) ? 0.0f : value;
			value = std::max(value, -600.0f);
			uint32_t height = ConvertHeightFloatToU16(value);

			auto curr = tmpbuf[iy * width + ix] * delta;
			auto vTop = Vector3(0.0f, (tmpbuf[clampIdx_Y(iy + 1) * width + ix] - curr), delta);
			auto vLeft = Vector3(-delta, (tmpbuf[iy * width + clampIdx_X(ix - 1)] - curr), 0.0f);
			auto vRight = Vector3(delta, (tmpbuf[iy * width + clampIdx_X(ix + 1)] - curr), 0.0f);
			auto vBottom = Vector3(0.0, (tmpbuf[clampIdx_Y(iy - 1) * width + ix] - curr), -delta);

			Vector3 normal = vLeft.crossProduct(vTop) + vBottom.crossProduct(vLeft) + vRight.crossProduct(vBottom) + vTop.crossProduct(vRight);
			normal.normalise();
			uint32_t slope = ConvertSlopeFloatToU8(normal.y);

			hmTmpData[idx] = height << 16 | slope << 8 /*| static_cast<uint8_t>(mask)*/;
		}
	}

	if (highResolution) {
		WriteHeightDataToFile(hmTmpData.data(), HeightMapPixPerPage, HeightMapPixPerPage, file_hm);
	}
	else {
		WriteHeightDataToFile(hmTmpData.data(), HeightMapPixPerPage, targetPixPerPage, file_hm);
	}

	fclose(file_hm);
	FreeImage_Unload(fibitmap);
}

void ConvertWorldHeightMapData(const std::string& filePath, const std::string& resPath, bool* selectedGrid, float KarstSize, int row, int col, int targetPixPerPage) {
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			int mapi = i + 1, mapj = j + 1;
			std::string rowString = mapi > 99 ? std::to_string(mapi) : mapi > 9 ? "0" + std::to_string(mapi) : "00" + std::to_string(mapi);
			std::string colString = mapj > 99 ? std::to_string(mapj) : mapj > 9 ? "0" + std::to_string(mapj) : "00" + std::to_string(mapj);
			std::string path = filePath + "worldmap3_" + colString + "_" + rowString + ".tif";

			std::string outFileName = "world" + std::to_string(i) + "_" + std::to_string(j) + ".hm";
			ConvertWorldHeightMapData(path, resPath + outFileName, selectedGrid[j * row + i], KarstSize, targetPixPerPage); //()
			/*FIBITMAP* fibitmap = FreeImage_Load(FIF_TIFF, path.c_str());

			int width = FreeImage_GetWidth(fibitmap);
			int height = FreeImage_GetHeight(fibitmap);

			std::vector<float> tmpbuf(width * height);
			int count = 0;
			for (int h = height - 1; h >= 0; --h, ++count) {
				BYTE* scanline = FreeImage_GetScanLine(fibitmap, h);
				float* _f32 = reinterpret_cast<float*>(scanline);
				for (int w = 0; w < width; ++w) {
					tmpbuf[count * width + w] = _f32[w];
				}
			}

			if (!CheckFileAvalible(tmpbuf)) {
				FreeImage_Unload(fibitmap);
				continue;
			}

			int HMPixVertexPerPage = (HeightMapPixPerPage + 3);
			int vertCount = HMPixVertexPerPage * HMPixVertexPerPage;

			auto clampIdx_X = [width](int num) {
				return std::max(std::min(num, (int)width - 1), 0);
				};

			auto clampIdx_Y = [height](int num) {
				return std::max(std::min(num, (int)height - 1), 0);
				};

			FILE* file_hm = fopen((resPath + outFileName).c_str(), "wb");

			std::vector<uint32_t> hmTmpData(HMPixVertexPerPage * HMPixVertexPerPage);

			float delta = KarstSize / HeightMapPixPerPage;

			for (int y = 0; y < HMPixVertexPerPage; y++) {
				for (int x = 0; x < HMPixVertexPerPage; x++) {
					int ix = x;
					int iy = y;
					int idx = y * HMPixVertexPerPage + x;
					float value = tmpbuf[iy * width + ix] / 8.f;
					value = isnan(value) ? 0.0f : value;
					value = std::max(value, -600.0f);
					uint32_t height = ConvertHeightFloatToU16(value);

					auto curr = tmpbuf[iy * width + ix] * delta;
					auto vTop = Vector3(0.0f, (tmpbuf[clampIdx_Y(iy + 1) * width + ix] - curr), delta);
					auto vLeft = Vector3(-delta, (tmpbuf[iy * width + clampIdx_X(ix - 1)] - curr), 0.0f);
					auto vRight = Vector3(delta, (tmpbuf[iy * width + clampIdx_X(ix + 1)] - curr), 0.0f);
					auto vBottom = Vector3(0.0, (tmpbuf[clampIdx_Y(iy - 1) * width + ix] - curr), -delta);

					Vector3 normal = vLeft.crossProduct(vTop) + vBottom.crossProduct(vLeft) + vRight.crossProduct(vBottom) + vTop.crossProduct(vRight);
					normal.normalise();
					uint32_t slope = ConvertSlopeFloatToU8(normal.y);

					hmTmpData[idx] = height << 16 | slope << 8
					//| static_cast<uint8_t>(mask)
			;
		}
			}

			if (selectedGrid[j * row + i]) {
				WriteHeightDataToFile(hmTmpData.data(), HeightMapPixPerPage, HeightMapPixPerPage, file_hm);
			}
			else {
				WriteHeightDataToFile(hmTmpData.data(), HeightMapPixPerPage, targetPixPerPage, file_hm);
			}
			fclose(file_hm);

			FreeImage_Unload(fibitmap);*/
		}
	}
}

void WriteTextureDataToFile(uint32_t* texData, int srcSize, int targetSize, FILE* file) {
	int scaleLevel = srcSize / targetSize;
	fwrite(&TexutreFileMagicID, sizeof(TexutreFileMagicID), 1, file);
	fwrite(&targetSize, sizeof(targetSize), 1, file);

	if (srcSize == targetSize) {
		int count = (srcSize + 1) * (srcSize + 1);
		fwrite(texData, sizeof(uint32_t), count, file);
		return;
	}

	int rowSize = targetSize + 1;
	std::vector<uint32_t> texTmpData(rowSize * rowSize);
	for (int y = 0; y < rowSize; y++) {
		for (int x = 0; x < rowSize; x++) {
			int idx = y * rowSize + x;
			texTmpData[idx] = texData[(scaleLevel * y) * (srcSize + 1) + (scaleLevel * x)];
		}
	}

	fwrite(texTmpData.data(), sizeof(uint32_t), rowSize * rowSize, file);
}

void ConvertWorldTextureMapData(const std::string& texfilePath, const std::string& heightfilePath, const std::string& resPath, bool* selectedGrid, int row, int col, int targetPixPerPage) {
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			int mapi = i + 1, mapj = j + 1;
			std::string rowString = mapi > 99 ? std::to_string(mapi) : mapi > 9 ? "0" + std::to_string(mapi) : "00" + std::to_string(mapi);
			std::string colString = mapj > 99 ? std::to_string(mapj) : mapj > 9 ? "0" + std::to_string(mapj) : "00" + std::to_string(mapj);
			std::string path = texfilePath + "worldtextrue_Clip1_" + colString + "_" + rowString + ".tif";
			FIBITMAP* fibitmap = FreeImage_Load(FIF_TIFF, path.c_str());

			FIBITMAP* bitmap32 = FreeImage_ConvertTo32Bits(fibitmap);
			if (!bitmap32) {
				std::cerr << "Failed to convert image to 32-bit!" << std::endl;
				FreeImage_Unload(fibitmap);
				continue;
			}

			path = heightfilePath + "worldmap_fill_" + colString + "_" + rowString + ".tif";
			FIBITMAP* hmbitmap = FreeImage_Load(FIF_TIFF, path.c_str());

			int width = FreeImage_GetWidth(hmbitmap);
			int height = FreeImage_GetHeight(hmbitmap);

			std::vector<float> tmpbuf(width * height);
			int count = 0;
			for (int h = height - 1; h >= 0; --h, ++count) {
				BYTE* scanline = FreeImage_GetScanLine(hmbitmap, h);
				float* _f32 = reinterpret_cast<float*>(scanline);
				for (int w = 0; w < width; ++w) {
					tmpbuf[count * width + w] = _f32[w];
				}
			}

			if (!CheckFileAvalible(tmpbuf)) {
				FreeImage_Unload(fibitmap);
				FreeImage_Unload(hmbitmap);
				FreeImage_Unload(bitmap32);
				continue;
			}

			width = FreeImage_GetWidth(fibitmap);
			height = FreeImage_GetHeight(fibitmap);
			BYTE* bits = FreeImage_GetBits(bitmap32);

			// 遍历每个像素并提取 RGBA 信息

			std::vector<std::uint32_t>  rgbaData(height * width);
			count = 0;
			for (int h = height - 1; h >= 0; --h, ++count) {
				for (int w = 0; w < width; w++) {
					BYTE* pixel = bits + (h * width + w) * 4; // 每个像素 4 字节
					BYTE red = pixel[FI_RGBA_RED];// 红色通道
					BYTE green = pixel[FI_RGBA_GREEN];// 绿色通道
					BYTE blue = pixel[FI_RGBA_BLUE];// 蓝色通道
					BYTE alpha = pixel[FI_RGBA_ALPHA];// Alpha 通道
					rgbaData[count * width + w] = pixel[FI_RGBA_RED] | (pixel[FI_RGBA_GREEN] << 8) | (pixel[FI_RGBA_BLUE] << 16) | (pixel[FI_RGBA_ALPHA] << 24);
				}
			}

			int TexturePixPage = (TexturePixPerPage + 1);
			int vertCount = TexturePixPage * TexturePixPage;

			auto clampIdx_X = [width](int num) {
				return std::max(std::min(num, (int)width - 1), 0);
				};

			auto clampIdx_Y = [height](int num) {
				return std::max(std::min(num, (int)height - 1), 0);
				};

			std::string outFileName = "world" + std::to_string(i) + "_" + std::to_string(j) + ".tex";
			FILE* file_tex;
			fopen_s(&file_tex, (resPath + outFileName).c_str(), "wb");

			std::vector<uint32_t> data(TexturePixPage * TexturePixPage);

			for (int y = 0; y < TexturePixPage; y++) {
				for (int x = 0; x < TexturePixPage; x++) {
					int idx = y * TexturePixPage + x;
					data[idx] = rgbaData[y * width + x];
				}
			}

			if (selectedGrid[j * row + i]) {
				WriteTextureDataToFile(data.data(), TexturePixPerPage, TexturePixPerPage, file_tex);
			}
			else {
				WriteTextureDataToFile(data.data(), TexturePixPerPage, targetPixPerPage, file_tex);
			}

			fclose(file_tex);
			FreeImage_Unload(fibitmap);
			FreeImage_Unload(hmbitmap);
			FreeImage_Unload(bitmap32);
		}
	}
}



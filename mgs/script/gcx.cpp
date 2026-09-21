#include "gcx.h"

Gcx::Gcx(std::string filename) {
	this->filename = filename;
	initData();
}

void Gcx::initData() {
	std::ifstream fs;
	int size = std::filesystem::file_size(filename);

	fs.open(filename, std::ios::binary);
	uint8_t* data = new uint8_t[size];
	fs.read((char*)data, size);
	this->gcxData = data;
	fs.close();
}

Gcx::~Gcx() {
	delete[] gcxData;
}

int Gcx::getNumProc() {
	return numProc;
}

void Gcx::setNumProc() {
	int i = 0;
	while (procTable[i] != -1) i++;
	numProc = i;
}

bool Gcx::isScriptResource(int idx) {
	return (resourceTable[idx] & 0xFF000000) != 0x80000000;
}

int Gcx::getResourceSize(int idx) {
	int offset = resourceTable[idx] & 0x00FFFFFF;
	int nextOffset = idx == numResource ? blockHeader->fontDataOffset : resourceTable[idx + 1] & 0x00FFFFFF;

	return nextOffset - offset;
}

int Gcx::getResourcesSize() {
	return blockHeader->fontDataOffset - blockHeader->stringResources;
}

int Gcx::getNumResource() {
	return numResource;
}

void Gcx::setNumResource() {
	numResource = (blockHeader->stringResources - blockHeader->resourceTableOffset) / 4;
}

GcxProc Gcx::getResource(int idx) {
	uint8_t* resources = &blockStart[blockHeader->stringResources];
	return &resources[resourceTable[idx] & 0x00FFFFFF];
}

void Gcx::decodeBuffer(uint32_t seed, uint8_t* src, int size) {
	for (int i = 0; i < size; i++) {
		seed = (seed * 0x7D2B89DD) + 0xCF9;
		int c = (seed >> 0xF) & 0x00FF;
		src[i] ^= c;
	}
}

void Gcx::decryptStringResources() {
	if (!blockHeader->seed) return;
	int size = blockHeader->fontDataOffset - blockHeader->stringResources;
	uint8_t* stringTableData = &blockStart[blockHeader->stringResources];
	if (!size) return;
	decodeBuffer(blockHeader->seed, stringTableData, size);
	blockHeader->seed = 0;
}

GcxProc Gcx::getMainProc() {
	return mainProcStart;
}

GcxProc Gcx::getProc(int idx) {
	return &procStart[procTable[idx] & 0x00FFFFFF];
}

void Gcx::setProc() {
	uint32_t mainProcOffset = *(uint32_t*)&blockStart[blockHeader->procOffset];

	procStart = &blockStart[blockHeader->procOffset + 4];
	mainProcSize = (uint32_t*)&procStart[mainProcOffset];
	mainProcStart = &procStart[mainProcOffset + 4];
}

bool Gcx::hasProcHashes() {
	return hashTable != nullptr;
}

uint32_t Gcx::getProcHash(int idx) {
	if (!hashTable) return 0;
	return hashTable[idx];
}

int Gcx::getProcIndexFromHash(uint32_t hash) {
	auto it = procHashMap.find(hash & 0x00FFFFFF);
	if (it != procHashMap.end()) {
		return it->second;
	}
	return -1;
}

void Gcx::open() {
	timestamp = (uint32_t*)gcxData;
	procTable = (int32_t*)&gcxData[4];
	setNumProc();

	int32_t* nextTable = &procTable[numProc + 1];
	GcxBlockHeader* candidateHeader = (GcxBlockHeader*)nextTable;

	procHashMap.clear();
	if (candidateHeader->resourceTableOffset == 0x14) {
		blockStart = (uint8_t*)nextTable;
		hashTable = nullptr;
	} else {
		hashTable = (uint32_t*)nextTable;
		int i = 0;
		while (nextTable[i] != -1) {
			procHashMap[nextTable[i] & 0x00FFFFFF] = i;
			i++;
		}
		blockStart = (uint8_t*)(&nextTable[i + 1]);
	}

	blockHeader = (GcxBlockHeader*)blockStart;
	resourceTable = (uint32_t*)&blockStart[blockHeader->resourceTableOffset];	
	setNumResource();
	decryptStringResources();
	setProc();
}

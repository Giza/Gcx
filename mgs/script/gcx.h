#pragma once
#include <fstream>
#include <filesystem>
#include <unordered_map>

typedef uint8_t* GcxProc;

struct GcxBlockHeader {
	uint32_t procOffset;
	uint32_t resourceTableOffset;
	uint32_t stringResources;
	uint32_t fontDataOffset;
	uint32_t seed;
};

class Gcx {
public:
	Gcx(std::string filename);
	~Gcx();

	void open();
	int getNumProc();
	GcxProc getMainProc();
	int getNumResource();
	int getResourcesSize();
	GcxProc getProc(int idx);
	int getResourceSize(int idx);
	GcxProc getResource(int idx);
	bool isScriptResource(int idx);
	uint32_t getProcHash(int idx);
	bool hasProcHashes();
	int getProcIndexFromHash(uint32_t hash);
private:
	std::string filename = "scenerio.gcx";

	int numProc = 0;
	uint8_t* gcxData;
	uint8_t* procStart;
	int32_t* procTable;
	uint32_t* hashTable = nullptr;
	std::unordered_map<uint32_t, int> procHashMap;
	int numResource = 0;
	uint32_t* timestamp;
	uint8_t* blockStart;
	uint32_t* mainProcSize;
	uint8_t*  mainProcStart;
	uint32_t* resourceTable;
	GcxBlockHeader* blockHeader;

	void setProc();
	void initData();
	void setNumProc();
	void setNumResource();
		
	void decryptStringResources();
	void decodeBuffer(uint32_t seed, uint8_t* src, int size);
};


#include <string>
#include <vector>
#include <iostream>
#include <filesystem>

#include "crosscore.hpp"

#define BUNDLE_SIG XD_FOURCC('B', 'N', 'D', 'L')

static void dbgmsg(const char* pMsg) {
	::printf("%s", pMsg);
}

static void init_sys() {
	sxSysIfc sysIfc;
	::memset(&sysIfc, 0, sizeof(sysIfc));
	sysIfc.fn_dbgmsg = dbgmsg;
	nxSys::init(&sysIfc);
}

struct FileInfo {
	uint32_t offs;
	uint32_t size;
};

struct Bundle {
	uint32_t sig;
	uint32_t nfiles;
};

struct SrcFile {
	std::string path;
	sxPackedData* pPk;
	void* pRaw;
	size_t size;
};

int bundle() {
	using namespace std;

	bool pathLstRaw = nxApp::get_bool_opt("rawlst", false);
	bool dataRaw = nxApp::get_bool_opt("raw", false);

	vector<string> flst;
	for (string fpath; getline(cin, fpath);) {
		//cout << flst.size() << ": " << fpath << endl;
		flst.push_back(fpath);
	}
	uint32_t nfiles = (uint32_t)flst.size();
	size_t pathLstRawSize = 0;
	for (auto fpath : flst) {
		pathLstRawSize += fpath.length() + 1;
	}
	cout << "num files: " << flst.size() << endl;
	cout << "path list raw size: " << pathLstRawSize << " bytes" << endl;
	char* pRawPathLst = (char*)nxCore::mem_alloc(pathLstRawSize, "fpaths");
	char* pRawPath = pRawPathLst;
	memset(pRawPathLst, 0, pathLstRawSize);
	for (auto fpath : flst) {
		memcpy(pRawPath, fpath.c_str(), fpath.length());
		pRawPath += fpath.length() + 1;
	}

	size_t pathLstSize = pathLstRawSize;
	char* pPathLstSrc = pRawPathLst;
	sxPackedData* pPathLstPk = nullptr;
	if (!pathLstRaw) {
		pPathLstPk = nxData::pack((const uint8_t*)pRawPathLst, pathLstRawSize, 2);
		if (pPathLstPk) {
			pPathLstSrc = (char*)pPathLstPk;
			pathLstSize = pPathLstPk->mPackSize;
		}
	}

	vector<SrcFile> srcFiles;
	size_t totalSizeRaw = 0;
	size_t totalSize = 0;
	for (auto fpath : flst) {
		SrcFile src;
		src.path = fpath;
		size_t rawSize = 0;
		uint8_t* pRaw = (uint8_t*)nxCore::bin_load(fpath.c_str(), &rawSize);
		totalSizeRaw += rawSize;
		cout << "+ " << fpath << ", " << rawSize << " bytes: ";
		sxPackedData* pPk = nullptr;
		if (!dataRaw) {
			pPk = nxData::pack(pRaw, rawSize, 2);
			if (!pPk) {
				pPk = nxData::pack(pRaw, rawSize, 0);
			}
		}
		if (pPk) {
			cout << "compressed to " << pPk->mPackSize << " bytes";
			src.pPk = pPk;
			src.pRaw = nullptr;
			src.size = pPk->mPackSize;
			nxCore::bin_unload(pRaw);
		} else {
			if (dataRaw) {
				cout << "storing uncompressed";
			} else {
				cout << "can't compress";
			}
			src.pPk = nullptr;
			src.pRaw = pRaw;
			src.size = rawSize;
		}
		totalSize += src.size;
		srcFiles.push_back(src);
		cout << endl;
	}

	size_t infoSize = sizeof(FileInfo) * nfiles;
	size_t bndSize = sizeof(Bundle) + infoSize + pathLstSize;
	for (auto src : srcFiles) {
		bndSize += src.size;
		//cout << ":: " << src.path << ": " << src.pPk << ", " << src.pRaw << ", " << src.size << endl;
	}

	Bundle* pBnd = (Bundle*)nxCore::mem_alloc(bndSize, "Bundle");
	memset(pBnd, 0, bndSize);
	FileInfo* pInfo = (FileInfo*)(pBnd + 1);

	cout << "total size (raw): " << totalSizeRaw << " bytes" << endl;
	cout << "      total size: " << totalSize << " bytes" << endl;

	pBnd->sig = BUNDLE_SIG;
	pBnd->nfiles = nfiles;
	void* pPathsDst = XD_INCR_PTR(pBnd + 1, infoSize);
	memcpy(pPathsDst, pPathLstSrc, pathLstSize);

	void* pDataDst = XD_INCR_PTR(pPathsDst, pathLstSize);
	FileInfo* pInfoDst = pInfo;
	uint32_t dataOffs = uint32_t(sizeof(Bundle) + infoSize + pathLstSize);
	for (auto src : srcFiles) {
		memcpy(pDataDst, src.pPk ? src.pPk : src.pRaw, src.size);
		pInfoDst->offs = dataOffs;
		pInfoDst->size = (uint32_t)src.size;
		dataOffs += pInfoDst->size;
		pDataDst = XD_INCR_PTR(pDataDst, pInfoDst->size);
		++pInfoDst;
	}

	const char* pOutPath = nxApp::get_opt("o");
	if (!pOutPath) {
		pOutPath = "bundle.bnd";
	}
	nxCore::bin_save(pOutPath, pBnd, bndSize);

	return 0;
}

int unbundle() {
	using namespace std;

	const char* pOutPath = nxApp::get_opt("o");
	if (!pOutPath) {
		nxCore::dbg_msg("To unbundle specify output path with -o:<path>.\n");
		return -1;
	}
	const char* pBndPath = nxApp::get_arg(0);
	size_t bndSize = 0;
	Bundle* pBnd = (Bundle*)nxCore::bin_load(pBndPath, &bndSize);
	if (!pBnd) {
		nxCore::dbg_msg("Bundle not found.\n");
		return -2;
	}
	if (bndSize < sizeof(Bundle) || pBnd->sig != BUNDLE_SIG || (int32_t)pBnd->nfiles <= 0) {
		nxCore::dbg_msg("Invalid bundle file format.\n");
		return -3;
	}

	FileInfo* pInfo = (FileInfo*)(pBnd + 1);
	char* pPathsTop = (char*)(pInfo + pBnd->nfiles);
	uint8_t* pUnpackedPaths = nxData::unpack((sxPackedData*)pPathsTop);
	if (pUnpackedPaths) {
		pPathsTop = (char*)pUnpackedPaths;
	}
	cout << "#files: " << pBnd->nfiles << endl;
	const char* pPathSrc = pPathsTop;
	string outBase(pOutPath);
	outBase += "/";
	for (uint32_t i = 0; i < pBnd->nfiles; ++i) {
		string path(pPathSrc);
		cout << i << ": " << path << endl;
		int sepPos = path.find("/");
		if (sepPos >= 0) {
			string dir = outBase + path.substr(0, sepPos);
			//cout << dir << endl;
			filesystem::create_directory(dir);
		}
		string outPath = outBase + path;
		nxCore::bin_save(outPath.c_str(), XD_INCR_PTR(pBnd, pInfo[i].offs), pInfo[i].size);
		pPathSrc += path.length() + 1;
	}

	return 0;
}

int main(int argc, char* argv[]) {
	nxApp::init_params(argc, argv);
	init_sys();

	bool res = 0;
	if (nxApp::get_args_count() > 0) {
		res = unbundle();
	} else {
		res = bundle();
	}

	nxApp::reset();
//	nxCore::mem_dbg();
	return res;
}

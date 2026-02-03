#pragma once
#include <filesystem>
#include <unordered_set>
#include <iostream>

#ifdef _WIN32
	#define NOMINMAX
	#include <wrl.h>
	// #include <dxcapi.h>
	#define CComPtr Microsoft::WRL::ComPtr
	#undef MemoryBarrier
	#include <dxc/dxcapi.h>
#else
	#include <dxc/dxcapi.h>
#endif

#include "utils.hpp"
#include "unicode.hpp"

// https://simoncoenen.com/blog/programming/graphics/DxcCompiling
// modified
class CustomIncludeHandler : public IDxcIncludeHandler {
	CComPtr<IDxcUtils> pUtils;

   public:
	CustomIncludeHandler(CComPtr<IDxcUtils> utils) : pUtils(utils) {}
	virtual ~CustomIncludeHandler() {};

	HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR							  pFilename,
										 _COM_Outptr_result_maybenull_ IDxcBlob **ppIncludeSource) override {
		CComPtr<IDxcBlobEncoding> pEncoding;
		std::error_code			  ec;
		std::filesystem::path	  path = std::filesystem::canonical(nri::WIDE_TO_UNICODE(pFilename), ec);
		if (ec) {
			dbLog(dbg::LOG_ERROR, "Failed to canonicalize include path: ", nri::WIDE_TO_UNICODE(pFilename),
				  " Error: ", ec.message());
			return E_FAIL;
		}

		if (IncludedFiles.find(path.string()) != IncludedFiles.end()) {
			static const char nullStr[] = " ";
			pUtils->CreateBlobFromPinned(nullStr, ARRAYSIZE(nullStr), DXC_CP_ACP, &pEncoding);
			*ppIncludeSource = pEncoding.Detach();
			return S_OK;
		}

		HRESULT hr = pUtils->LoadFile(pFilename, nullptr, &pEncoding);
		if (SUCCEEDED(hr)) {
			IncludedFiles.insert(path.string());
			*ppIncludeSource = pEncoding.Detach();
		} else {
			dbLog(dbg::LOG_ERROR, "Failed to load include file: ", nri::WIDE_TO_UNICODE(pFilename), " HR: ", hr);
		}
		return hr;
	}

	void reset() { IncludedFiles.clear(); }

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **) override { return E_NOINTERFACE; }
	ULONG STDMETHODCALLTYPE	  AddRef(void) override { return 0; }
	ULONG STDMETHODCALLTYPE	  Release(void) override { return 0; }

	std::unordered_set<std::string> IncludedFiles;
};

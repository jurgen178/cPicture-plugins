#pragma once

#include <afxwin.h>
#include "json.hpp"
#include <string>

namespace PluginShared
{
	std::string CStringToUtf8(const CString& text);
	CString Utf8ToCString(const std::string& text);
	CString AppendToBaseName(const CString& file_name, const CString& append);
	CString GetCurrentModuleDirectory();
	CString GetPluginSettingsFilePath();

	class PluginSettingsSection
	{
	public:
		explicit PluginSettingsSection(const CString& plugin_name);
		~PluginSettingsSection();

		CString GetString(const CString& key, const CString& default_value) const;
		int GetInt(const CString& key, int default_value) const;
		float GetFloat(const CString& key, float default_value) const;
		bool GetBool(const CString& key, bool default_value) const;
		bool GetBinaryData(const CString& key, void* data, int size) const;

		void SetString(const CString& key, const CString& value, const CString& default_value);
		void SetInt(const CString& key, int value, int default_value);
		void SetFloat(const CString& key, float value, float default_value);
		void SetBool(const CString& key, bool value, bool default_value);
		void SetBinaryData(const CString& key, const void* data, int size);
		void Remove(const CString& key);

		bool Save();
		bool HasParseError() const;
		const CString& GetFilePath() const;

	private:
		void EnsureLoaded() const;
		nlohmann::json* GetOrCreateSection();
		nlohmann::json* GetSection() const;
		void CleanupSectionIfEmpty();

		CString plugin_name_;
		CString file_path_;
		mutable bool is_loaded_;
		mutable bool has_parse_error_;
		mutable nlohmann::json* root_value_;
		bool is_dirty_;
	};
}
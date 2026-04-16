#include "PluginSettings.h"

#include "parson.h"

#include <stdio.h>

namespace
{
	CString CombinePath(const CString& left, const CString& right)
	{
		if (left.IsEmpty())
			return right;
		if (left[left.GetLength() - 1] == L'\\')
			return left + right;
		return left + L"\\" + right;
	}

	bool ReadUtf8TextFile(const CString& file_path, CString& text)
	{
		FILE* file = NULL;
		if (_wfopen_s(&file, file_path, L"rb") != 0 || file == NULL)
			return false;

		fseek(file, 0, SEEK_END);
		const long length = ftell(file);
		fseek(file, 0, SEEK_SET);

		std::string utf8;
		if (length > 0)
		{
			utf8.resize(length);
			const size_t bytes_read = fread(&utf8[0], 1, utf8.size(), file);
			utf8.resize(bytes_read);
		}

		fclose(file);

		if (utf8.size() >= 3
			&& static_cast<unsigned char>(utf8[0]) == 0xEF
			&& static_cast<unsigned char>(utf8[1]) == 0xBB
			&& static_cast<unsigned char>(utf8[2]) == 0xBF)
		{
			utf8.erase(0, 3);
		}

		text = PluginShared::Utf8ToCString(utf8);
		return true;
	}

	bool WriteUtf8TextFile(const CString& file_path, const std::string& utf8)
	{
		FILE* file = NULL;
		if (_wfopen_s(&file, file_path, L"wb") != 0 || file == NULL)
			return false;

		const size_t bytes_written = utf8.empty() ? 0 : fwrite(utf8.data(), 1, utf8.size(), file);
		fclose(file);
		return bytes_written == utf8.size();
	}

	bool ReplaceFileAtomically(const CString& source_file, const CString& target_file)
	{
		for (int attempt = 0; attempt < 3; ++attempt)
		{
			if (::MoveFileExW(source_file, target_file, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
				return true;
		}

		::DeleteFileW(source_file);
		return false;
	}

	std::wstring ToWide(const CString& text)
	{
		return std::wstring(text.GetString());
	}

	CString ToCString(const std::wstring& text)
	{
		return CString(text.c_str());
	}

	CString BuildTempFilePath(const CString& file_path)
	{
		return file_path + L".tmp";
	}
}

namespace PluginShared
{
	std::string CStringToUtf8(const CString& text)
	{
		if (text.IsEmpty())
			return std::string();

		const int length = ::WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
		if (length <= 1)
			return std::string();

		std::string utf8(length, '\0');
		::WideCharToMultiByte(CP_UTF8, 0, text, -1, &utf8[0], length, NULL, NULL);
		utf8.resize(length - 1);
		return utf8;
	}

	CString Utf8ToCString(const std::string& text)
	{
		if (text.empty())
			return CString();

		const int length = ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), NULL, 0);
		if (length <= 0)
			return CString();

		CString wide;
		wchar_t* buffer = wide.GetBuffer(length);
		::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), buffer, length);
		wide.ReleaseBuffer(length);
		return wide;
	}

	CString AppendToBaseName(const CString& file_name, const CString& append)
	{
		const int extension_dot = file_name.ReverseFind(L'.');
		if (extension_dot < 0)
			return file_name + append;
		return file_name.Left(extension_dot) + append + file_name.Mid(extension_dot);
	}

	CString GetCurrentModuleDirectory()
	{
		HMODULE module_handle = NULL;
		if (!::GetModuleHandleExW(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(&GetCurrentModuleDirectory),
			&module_handle))
		{
			return CString();
		}

		WCHAR module_path[MAX_PATH] = {};
		::GetModuleFileNameW(module_handle, module_path, MAX_PATH);
		CString directory(module_path);
		const int slash = directory.ReverseFind(L'\\');
		if (slash >= 0)
			directory = directory.Left(slash);
		return directory;
	}

	CString GetPluginSettingsFilePath()
	{
		return CombinePath(GetCurrentModuleDirectory(), L"plugin-settings.json");
	}

	PluginSettingsSection::PluginSettingsSection(const CString& plugin_name)
		: plugin_name_(plugin_name),
		  file_path_(GetPluginSettingsFilePath()),
		  is_loaded_(false),
		  has_parse_error_(false),
		  root_value_(NULL),
		  is_dirty_(false)
	{
	}

	PluginSettingsSection::~PluginSettingsSection()
	{
		if (root_value_ != NULL)
			json_value_free(root_value_);
	}

	CString PluginSettingsSection::GetString(const CString& key, const CString& default_value) const
	{
		json_object_t* section = GetSection();
		if (section == NULL)
			return default_value;

		const std::string key_utf8 = CStringToUtf8(key);
		const char* value = json_object_get_string(section, key_utf8.c_str());
		return value != NULL ? Utf8ToCString(std::string(value)) : default_value;
	}

	int PluginSettingsSection::GetInt(const CString& key, const int default_value) const
	{
		json_object_t* section = GetSection();
		if (section == NULL)
			return default_value;

		const std::string key_utf8 = CStringToUtf8(key);
		if (!json_object_has_value_of_type(section, key_utf8.c_str(), JSONNumber))
			return default_value;

		return static_cast<int>(json_object_get_number(section, key_utf8.c_str()));
	}

	bool PluginSettingsSection::GetBool(const CString& key, const bool default_value) const
	{
		json_object_t* section = GetSection();
		if (section == NULL)
			return default_value;

		const std::string key_utf8 = CStringToUtf8(key);
		if (!json_object_has_value_of_type(section, key_utf8.c_str(), JSONBoolean))
			return default_value;

		return json_object_get_boolean(section, key_utf8.c_str()) == 1;
	}

	void PluginSettingsSection::SetString(const CString& key, const CString& value, const CString& default_value)
	{
		EnsureLoaded();
		if (has_parse_error_)
			return;
		if (value == default_value)
		{
			Remove(key);
			return;
		}

		json_object_t* section = GetOrCreateSection();
		if (section == NULL)
			return;

		const CString current_value = GetString(key, CString(L"\x1"));
		if (current_value == value)
			return;

		const std::string key_utf8 = CStringToUtf8(key);
		const std::string value_utf8 = CStringToUtf8(value);
		if (json_object_set_string(section, key_utf8.c_str(), value_utf8.c_str()) == JSONSuccess)
			is_dirty_ = true;
	}

	void PluginSettingsSection::SetInt(const CString& key, const int value, const int default_value)
	{
		EnsureLoaded();
		if (has_parse_error_)
			return;
		if (value == default_value)
		{
			Remove(key);
			return;
		}

		json_object_t* section = GetOrCreateSection();
		if (section == NULL)
			return;

		if (GetInt(key, INT_MIN) == value)
			return;

		const std::string key_utf8 = CStringToUtf8(key);
		if (json_object_set_number(section, key_utf8.c_str(), static_cast<double>(value)) == JSONSuccess)
			is_dirty_ = true;
	}

	void PluginSettingsSection::SetBool(const CString& key, const bool value, const bool default_value)
	{
		EnsureLoaded();
		if (has_parse_error_)
			return;
		if (value == default_value)
		{
			Remove(key);
			return;
		}

		json_object_t* section = GetOrCreateSection();
		if (section == NULL)
			return;

		if (GetBool(key, !value) == value)
			return;

		const std::string key_utf8 = CStringToUtf8(key);
		if (json_object_set_boolean(section, key_utf8.c_str(), value ? 1 : 0) == JSONSuccess)
			is_dirty_ = true;
	}

	void PluginSettingsSection::Remove(const CString& key)
	{
		EnsureLoaded();
		if (has_parse_error_ || root_value_ == NULL || json_value_get_type(root_value_) != JSONObject)
			return;

		json_object_t* section = GetSection();
		if (section == NULL)
			return;

		const std::string key_utf8 = CStringToUtf8(key);
		if (!json_object_has_value(section, key_utf8.c_str()))
			return;

		json_object_remove(section, key_utf8.c_str());
		is_dirty_ = true;
		CleanupSectionIfEmpty();
	}

	bool PluginSettingsSection::Save()
	{
		EnsureLoaded();
		if (has_parse_error_)
			return false;
		if (!is_dirty_)
			return true;

		if (root_value_ == NULL)
			root_value_ = json_value_init_object();

		char* serialized_string = json_serialize_to_string_pretty(root_value_);
		if (serialized_string == NULL)
			return false;

		const CString temp_file_path = BuildTempFilePath(file_path_);
		const std::string utf8(serialized_string);
		json_free_serialized_string(serialized_string);

		if (!WriteUtf8TextFile(temp_file_path, utf8))
			return false;

		if (!ReplaceFileAtomically(temp_file_path, file_path_))
			return false;

		is_dirty_ = false;
		return true;
	}

	bool PluginSettingsSection::HasParseError() const
	{
		EnsureLoaded();
		return has_parse_error_;
	}

	const CString& PluginSettingsSection::GetFilePath() const
	{
		return file_path_;
	}

	void PluginSettingsSection::EnsureLoaded() const
	{
		if (is_loaded_)
			return;

		is_loaded_ = true;
		has_parse_error_ = false;
		root_value_ = json_value_init_object();

		CString file_text;
		if (!ReadUtf8TextFile(file_path_, file_text))
			return;

		if (root_value_ != NULL)
		{
			json_value_free(root_value_);
			root_value_ = NULL;
		}

		const std::string utf8 = CStringToUtf8(file_text);
		root_value_ = json_parse_string(utf8.c_str());
		if (root_value_ == NULL || json_value_get_type(root_value_) != JSONObject)
		{
			has_parse_error_ = true;
			if (root_value_ != NULL)
			{
				json_value_free(root_value_);
				root_value_ = NULL;
			}
			root_value_ = json_value_init_object();
			return;
		}
	}

	json_object_t* PluginSettingsSection::GetOrCreateSection()
	{
		EnsureLoaded();
		if (has_parse_error_ || root_value_ == NULL)
			return NULL;

		JSON_Object* root_object = json_value_get_object(root_value_);
		const std::string plugin_name_utf8 = CStringToUtf8(plugin_name_);
		JSON_Value* section_value = json_object_get_value(root_object, plugin_name_utf8.c_str());
		if (section_value != NULL && json_value_get_type(section_value) == JSONObject)
			return json_value_get_object(section_value);

		if (section_value != NULL)
		{
			json_object_remove(root_object, plugin_name_utf8.c_str());
			is_dirty_ = true;
		}

		JSON_Value* new_section = json_value_init_object();
		if (new_section == NULL)
			return NULL;

		if (json_object_set_value(root_object, plugin_name_utf8.c_str(), new_section) != JSONSuccess)
		{
			json_value_free(new_section);
			return NULL;
		}

		is_dirty_ = true;
		return json_value_get_object(new_section);
	}

	json_object_t* PluginSettingsSection::GetSection() const
	{
		EnsureLoaded();
		if (has_parse_error_ || root_value_ == NULL || json_value_get_type(root_value_) != JSONObject)
			return NULL;

		JSON_Object* root_object = json_value_get_object(root_value_);
		const std::string plugin_name_utf8 = CStringToUtf8(plugin_name_);
		JSON_Value* section_value = json_object_get_value(root_object, plugin_name_utf8.c_str());
		if (section_value == NULL || json_value_get_type(section_value) != JSONObject)
			return NULL;

		return json_value_get_object(section_value);
	}

	void PluginSettingsSection::CleanupSectionIfEmpty()
	{
		if (root_value_ == NULL || json_value_get_type(root_value_) != JSONObject)
			return;

		JSON_Object* root_object = json_value_get_object(root_value_);
		const std::string plugin_name_utf8 = CStringToUtf8(plugin_name_);
		JSON_Value* section_value = json_object_get_value(root_object, plugin_name_utf8.c_str());
		if (section_value == NULL || json_value_get_type(section_value) != JSONObject)
			return;

		JSON_Object* section = json_value_get_object(section_value);
		if (json_object_get_count(section) == 0)
			json_object_remove(root_object, plugin_name_utf8.c_str());
	}
}
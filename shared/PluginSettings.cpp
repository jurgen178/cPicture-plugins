#include "stdafx.h"
#include "PluginSettings.h"

#include <math.h>
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

	CString BuildTempFilePath(const CString& file_path)
	{
		return file_path + L".tmp";
	}

	bool ReadUtf8FileBytes(const CString& file_path, std::string& utf8)
	{
		utf8.clear();

		FILE* file = NULL;
		if (_wfopen_s(&file, file_path, L"rb") != 0 || file == NULL)
			return false;

		fseek(file, 0, SEEK_END);
		const long length = ftell(file);
		fseek(file, 0, SEEK_SET);

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

		return true;
	}

	CString EncodeBinaryData(const BYTE* data, const int size)
	{
		static const WCHAR hex_digits[] = L"0123456789abcdef";
		CString encoded;
		for (int index = 0; index < size; ++index)
		{
			const BYTE value = data[index];
			encoded += hex_digits[(value >> 4) & 0x0F];
			encoded += hex_digits[value & 0x0F];
		}

		return encoded;
	}

	bool DecodeBinaryData(const CString& hex, void* data, const int size)
	{
		if (data == NULL || size < 0 || (hex.GetLength() % 2) != 0)
			return false;

		BYTE* bytes = static_cast<BYTE*>(data);
		const int decoded_size = hex.GetLength() / 2;
		for (int index = 0; index < decoded_size; ++index)
		{
			int value = 0;
			for (int digit = 0; digit < 2; ++digit)
			{
				value <<= 4;
				const WCHAR c = hex[index * 2 + digit];

				if (c >= L'0' && c <= L'9')
					value += c - L'0';
				else if (c >= L'a' && c <= L'f')
					value += c - L'a' + 10;
				else if (c >= L'A' && c <= L'F')
					value += c - L'A' + 10;
				else
					return false;
			}

			if (index < size)
				bytes[index] = static_cast<BYTE>(value);
		}

		return true;
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
		HMODULE module_handle = AfxGetInstanceHandle();
		if (module_handle == NULL)
			return CString();

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
			delete root_value_;
	}

	CString PluginSettingsSection::GetString(const CString& key, const CString& default_value) const
	{
		nlohmann::json* section = GetSection();
		if (section == NULL)
			return default_value;

		const std::string key_utf8 = CStringToUtf8(key);
		nlohmann::json::const_iterator it = section->find(key_utf8);
		if (it == section->end() || !it->is_string())
			return default_value;

		return Utf8ToCString(it->get<std::string>());
	}

	int PluginSettingsSection::GetInt(const CString& key, const int default_value) const
	{
		nlohmann::json* section = GetSection();
		if (section == NULL)
			return default_value;

		const std::string key_utf8 = CStringToUtf8(key);
		nlohmann::json::const_iterator it = section->find(key_utf8);
		if (it == section->end() || !it->is_number())
			return default_value;

		return static_cast<int>(it->get<double>());
	}

	float PluginSettingsSection::GetFloat(const CString& key, const float default_value) const
	{
		nlohmann::json* section = GetSection();
		if (section == NULL)
			return default_value;

		const std::string key_utf8 = CStringToUtf8(key);
		nlohmann::json::const_iterator it = section->find(key_utf8);
		if (it == section->end() || !it->is_number())
			return default_value;

		return static_cast<float>(it->get<double>());
	}

	bool PluginSettingsSection::GetBool(const CString& key, const bool default_value) const
	{
		nlohmann::json* section = GetSection();
		if (section == NULL)
			return default_value;

		const std::string key_utf8 = CStringToUtf8(key);
		nlohmann::json::const_iterator it = section->find(key_utf8);
		if (it == section->end() || !it->is_boolean())
			return default_value;

		return it->get<bool>();
	}

	bool PluginSettingsSection::GetBinaryData(const CString& key, void* data, const int size) const
	{
		nlohmann::json* section = GetSection();
		if (section == NULL)
			return false;

		const std::string key_utf8 = CStringToUtf8(key);
		nlohmann::json::const_iterator it = section->find(key_utf8);
		if (it == section->end() || !it->is_string())
			return false;

		return DecodeBinaryData(Utf8ToCString(it->get<std::string>()), data, size);
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

		nlohmann::json* section = GetOrCreateSection();
		if (section == NULL)
			return;

		const CString current_value = GetString(key, CString(L"\x1"));
		if (current_value == value)
			return;

		const std::string key_utf8 = CStringToUtf8(key);
		(*section)[key_utf8] = CStringToUtf8(value);
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

		nlohmann::json* section = GetOrCreateSection();
		if (section == NULL)
			return;

		if (GetInt(key, INT_MIN) == value)
			return;

		const std::string key_utf8 = CStringToUtf8(key);
		(*section)[key_utf8] = value;
		is_dirty_ = true;
	}

	void PluginSettingsSection::SetFloat(const CString& key, const float value, const float default_value)
	{
		EnsureLoaded();
		if (has_parse_error_)
			return;
		if (value == default_value)
		{
			Remove(key);
			return;
		}

		nlohmann::json* section = GetOrCreateSection();
		if (section == NULL)
			return;

		const std::string key_utf8 = CStringToUtf8(key);
		nlohmann::json::const_iterator it = section->find(key_utf8);
		if (it != section->end() && it->is_number() && static_cast<float>(it->get<double>()) == value)
			return;

		(*section)[key_utf8] = value;
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

		nlohmann::json* section = GetOrCreateSection();
		if (section == NULL)
			return;

		if (GetBool(key, !value) == value)
			return;

		const std::string key_utf8 = CStringToUtf8(key);
		(*section)[key_utf8] = value;
		is_dirty_ = true;
	}

	void PluginSettingsSection::SetBinaryData(const CString& key, const void* data, const int size)
	{
		EnsureLoaded();
		if (has_parse_error_ || data == NULL || size <= 0)
			return;

		nlohmann::json* section = GetOrCreateSection();
		if (section == NULL)
			return;

		const CString encoded(EncodeBinaryData(static_cast<const BYTE*>(data), size));
		const CString current_value(GetString(key, CString()));
		if (current_value == encoded)
			return;

		const std::string key_utf8 = CStringToUtf8(key);
		(*section)[key_utf8] = CStringToUtf8(encoded);
		is_dirty_ = true;
	}

	void PluginSettingsSection::Remove(const CString& key)
	{
		EnsureLoaded();
		if (has_parse_error_ || root_value_ == NULL || !root_value_->is_object())
			return;

		nlohmann::json* section = GetSection();
		if (section == NULL)
			return;

		const std::string key_utf8 = CStringToUtf8(key);
		if (!section->contains(key_utf8))
			return;

		section->erase(key_utf8);
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
			root_value_ = new nlohmann::json(nlohmann::json::object());

		const CString temp_file_path = BuildTempFilePath(file_path_);
		const std::string utf8 = root_value_->dump(4, ' ', false, nlohmann::json::error_handler_t::strict);

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
		root_value_ = new nlohmann::json(nlohmann::json::object());

		std::string utf8;
		if (!ReadUtf8FileBytes(file_path_, utf8))
			return;

		if (root_value_ != NULL)
		{
			delete root_value_;
			root_value_ = NULL;
		}

		root_value_ = new nlohmann::json(nlohmann::json::parse(utf8, NULL, false));
		if (root_value_ == NULL || root_value_->is_discarded() || !root_value_->is_object())
		{
			has_parse_error_ = true;
			if (root_value_ != NULL)
			{
				delete root_value_;
				root_value_ = NULL;
			}
			root_value_ = new nlohmann::json(nlohmann::json::object());
			return;
		}
	}

	nlohmann::json* PluginSettingsSection::GetOrCreateSection()
	{
		EnsureLoaded();
		if (has_parse_error_ || root_value_ == NULL)
			return NULL;

		const std::string plugin_name_utf8 = CStringToUtf8(plugin_name_);
		nlohmann::json::iterator it = root_value_->find(plugin_name_utf8);
		if (it != root_value_->end() && it->is_object())
			return &(*it);

		if (it != root_value_->end())
		{
			root_value_->erase(plugin_name_utf8);
			is_dirty_ = true;
		}

		(*root_value_)[plugin_name_utf8] = nlohmann::json::object();
		is_dirty_ = true;
		return &(*root_value_)[plugin_name_utf8];
	}

	nlohmann::json* PluginSettingsSection::GetSection() const
	{
		EnsureLoaded();
		if (has_parse_error_ || root_value_ == NULL || !root_value_->is_object())
			return NULL;

		const std::string plugin_name_utf8 = CStringToUtf8(plugin_name_);
		nlohmann::json::iterator it = root_value_->find(plugin_name_utf8);
		if (it == root_value_->end() || !it->is_object())
			return NULL;

		return &(*it);
	}

	void PluginSettingsSection::CleanupSectionIfEmpty()
	{
		if (root_value_ == NULL || !root_value_->is_object())
			return;

		const std::string plugin_name_utf8 = CStringToUtf8(plugin_name_);
		nlohmann::json::iterator it = root_value_->find(plugin_name_utf8);
		if (it == root_value_->end() || !it->is_object())
			return;

		if (it->empty())
			root_value_->erase(it);
	}
}
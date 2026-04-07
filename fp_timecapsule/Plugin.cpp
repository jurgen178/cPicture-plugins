#include "stdafx.h"
#include "Plugin.h"
#include "SettingsDlg.h"

#include <array>
#include <algorithm>
#include <atomic>
#include <cstdarg>
#include <cmath>
#include <cwctype>
#include <map>
#include <mutex>
#include <process.h>
#include <shlobj.h>
#include <string>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")


namespace
{
	enum SORT_MODE
	{
		SORT_MODE_EXIF = 0,
		SORT_MODE_SELECTION,
		SORT_MODE_GPS_WEST_EAST,
		SORT_MODE_GPS_NORTH_SOUTH,
		SORT_MODE_GPS_EAST_WEST,
		SORT_MODE_GPS_SOUTH_NORTH,
	};

	constexpr int header_height = 138;
	constexpr int compact_header_height = 72;
	constexpr int summary_height = 66;
	constexpr int tile_padding = 18;
	constexpr int caption_height = 66;
	constexpr DWORD throttled_lookup_delay_ms = 1000;
	constexpr DWORD worker_poll_interval_ms = 100;

	struct GeoPoint
	{
		bool valid = false;
		double latitude = 0.0;
		double longitude = 0.0;
	};

	struct RouteCluster
	{
		double latitude = 0.0;
		double longitude = 0.0;
		int count = 0;
		COLORREF color = RGB(0, 0, 0);
		CString place_name;
	};

	struct PosterEntry
	{
		const picture_data* picture = nullptr;
		GeoPoint geo;
		int cluster_index = -1;
		double hop_distance_km = 0.0;
	};

	struct PosterInsights
	{
		vector<PosterEntry> entries;
		vector<RouteCluster> clusters;
		int gps_picture_count = 0;
		double total_distance_km = 0.0;
		double longest_hop_km = 0.0;
		int longest_hop_index = -1;
		__int64 first_exif_time = 0;
		__int64 last_exif_time = 0;
	};

	class ScopedWaitCursor
	{
	public:
		ScopedWaitCursor()
			: previous_cursor(::SetCursor(::LoadCursor(nullptr, IDC_WAIT)))
		{
		}

		~ScopedWaitCursor()
		{
			::SetCursor(previous_cursor != nullptr ? previous_cursor : ::LoadCursor(nullptr, IDC_ARROW));
		}

	private:
		HCURSOR previous_cursor = nullptr;
	};

	CString LoadStringResource(const UINT resource_id)
	{
		// Centralize resource lookups so newly added UI text stays out of the code paths.
		CString text;
		text.LoadString(resource_id);
		return text;
	}

	CString FormatStringResource(const UINT resource_id, ...)
	{
		// Formatted status text is also loaded from resources to keep DE/EN variants in the RC file.
		CString format(LoadStringResource(resource_id));
		CString text;
		va_list args;
		va_start(args, resource_id);
		text.FormatV(format, args);
		va_end(args);
		return text;
	}

	CString GetFileNameOnly(const CString& file_name)
	{
		// The plugin receives absolute file paths from cPicture, but captions and stems only need the leaf name.
		const int pos = file_name.ReverseFind(L'\\');
		return pos >= 0 ? file_name.Mid(pos + 1) : file_name;
	}

	CString GetBaseName(const CString& file_name)
	{
		CString name(GetFileNameOnly(file_name));
		const int dot = name.ReverseFind(L'.');
		return dot > 0 ? name.Left(dot) : name;
	}

	CString GetDirectory(const CString& file_name)
	{
		const int pos = file_name.ReverseFind(L'\\');
		return pos >= 0 ? file_name.Left(pos + 1) : CString();
	}

	CString GetExtension(const CString& file_name)
	{
		const int dot = file_name.ReverseFind(L'.');
		return dot >= 0 ? file_name.Mid(dot) : L".jpg";
	}

	CString SanitizeStem(CString text)
	{
		text.Trim();
		if (text.IsEmpty())
			text = L"timecapsule";

		for (int index = 0; index < text.GetLength(); ++index)
		{
			const WCHAR ch = text[index];
			if (ch == L' ')
				text.SetAt(index, L'-');
			else if (wcschr(L"\\/:*?\"<>|", ch) != nullptr)
				text.SetAt(index, L'_');
		}

		return text;
	}

	COLORREF GetClusterColor(const int index)
	{
		static const std::array<COLORREF, 8> colors =
		{
			RGB(227, 102, 73),
			RGB(70, 130, 180),
			RGB(72, 156, 92),
			RGB(209, 147, 46),
			RGB(137, 92, 190),
			RGB(47, 163, 168),
			RGB(196, 76, 142),
			RGB(124, 114, 84)
		};

		return colors[index % colors.size()];
	}

	struct WinHttpHandle
	{
		WinHttpHandle() = default;
		explicit WinHttpHandle(HINTERNET value) : handle(value) {}
		WinHttpHandle(const WinHttpHandle&) = delete;
		WinHttpHandle& operator=(const WinHttpHandle&) = delete;
		WinHttpHandle(WinHttpHandle&& other) noexcept : handle(other.handle)
		{
			other.handle = nullptr;
		}
		WinHttpHandle& operator=(WinHttpHandle&& other) noexcept
		{
			if (this != &other)
			{
				if (handle != nullptr)
					::WinHttpCloseHandle(handle);
				handle = other.handle;
				other.handle = nullptr;
			}

			return *this;
		}
		~WinHttpHandle()
		{
			Close();
		}

		void Close()
		{
			if (handle != nullptr)
			{
				::WinHttpCloseHandle(handle);
				handle = nullptr;
			}
		}

		explicit operator bool() const
		{
			return handle != nullptr;
		}

		operator HINTERNET() const
		{
			return handle;
		}

		HINTERNET handle = nullptr;
	};

	class ScopedKernelHandle
	{
	public:
		ScopedKernelHandle() = default;
		explicit ScopedKernelHandle(HANDLE value) : handle(value) {}
		ScopedKernelHandle(const ScopedKernelHandle&) = delete;
		ScopedKernelHandle& operator=(const ScopedKernelHandle&) = delete;
		ScopedKernelHandle(ScopedKernelHandle&& other) noexcept : handle(other.handle)
		{
			other.handle = nullptr;
		}
		ScopedKernelHandle& operator=(ScopedKernelHandle&& other) noexcept
		{
			if (this != &other)
			{
				Reset();
				handle = other.handle;
				other.handle = nullptr;
			}

			return *this;
		}
		~ScopedKernelHandle()
		{
			Reset();
		}

		void Reset(HANDLE value = nullptr)
		{
			if (handle != nullptr)
				::CloseHandle(handle);
			handle = value;
		}

		HANDLE Get() const
		{
			return handle;
		}

		explicit operator bool() const
		{
			return handle != nullptr;
		}

	private:
		HANDLE handle = nullptr;
	};

	// One shared cancellation object coordinates UI cancel requests, throttle waits and active HTTP requests.
	class CancellationContext
	{
	public:
		CancellationContext()
			: cancel_event(::CreateEvent(nullptr, TRUE, FALSE, nullptr))
		{
		}

		bool IsValid() const
		{
			return static_cast<bool>(cancel_event);
		}

		bool IsCanceled() const
		{
			return canceled.load();
		}

		HANDLE GetEvent() const
		{
			return cancel_event.Get();
		}

		void RequestCancel()
		{
			canceled.store(true);
			if (cancel_event)
				::SetEvent(cancel_event.Get());

			std::lock_guard<std::mutex> lock(active_request_mutex);
			if (active_request != nullptr)
			{
				active_request->Close();
				active_request = nullptr;
			}
		}

		bool RegisterRequest(WinHttpHandle& request)
		{
			if (IsCanceled())
				return false;

			std::lock_guard<std::mutex> lock(active_request_mutex);
			if (IsCanceled() || !request)
				return false;

			active_request = &request;
			return true;
		}

		void ClearRequest(WinHttpHandle& request)
		{
			std::lock_guard<std::mutex> lock(active_request_mutex);
			if (active_request == &request)
				active_request = nullptr;
		}

	private:
		std::atomic<bool> canceled = false;
		ScopedKernelHandle cancel_event;
		std::mutex active_request_mutex;
		WinHttpHandle* active_request = nullptr;
	};

	// The current WinHTTP request is registered so a cancel can break an in-flight network call immediately.
	class ActiveRequestScope
	{
	public:
		ActiveRequestScope(CancellationContext* context_value, WinHttpHandle& request_value)
			: context(context_value), request(&request_value)
		{
			registered = context == nullptr ? true : context->RegisterRequest(request_value);
		}

		~ActiveRequestScope()
		{
			if (registered && context != nullptr && request != nullptr)
				context->ClearRequest(*request);
		}

		bool IsRegistered() const
		{
			return registered;
		}

	private:
		CancellationContext* context = nullptr;
		WinHttpHandle* request = nullptr;
		bool registered = false;
	};

	// The shell progress dialog is COM-based, so the UI thread explicitly initializes COM for this phase.
	class ScopedComInitializer
	{
	public:
		ScopedComInitializer()
		{
			result = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
			initialized = SUCCEEDED(result);
			if (result == RPC_E_CHANGED_MODE)
				result = S_OK;
		}

		~ScopedComInitializer()
		{
			if (initialized)
				::CoUninitialize();
		}

		bool IsAvailable() const
		{
			return SUCCEEDED(result);
		}

	private:
		HRESULT result = E_FAIL;
		bool initialized = false;
	};

	// The worker only writes progress state; the UI thread reads and renders it.
	struct ProgressState
	{
		std::mutex mutex;
		CString phase;
		CString detail;
		CString detail2;
		ULONGLONG completed = 0;
		ULONGLONG total = 1;
		unsigned long revision = 0;
	};

	// Wrapper around the standard Windows progress dialog to keep UI code small and predictable.
	class ProgressDialog
	{
	public:
		~ProgressDialog()
		{
			Destroy();
		}

		bool Create(HWND owner, const CString& title, const CString& cancel_message)
		{
			HRESULT result = ::CoCreateInstance(CLSID_ProgressDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
			if (FAILED(result) || dialog == nullptr)
				return false;

			dialog->SetTitle(title);
			dialog->SetCancelMsg(cancel_message, nullptr);
			dialog->StartProgressDialog(owner, nullptr, PROGDLG_NOMINIMIZE, nullptr);
			return true;
		}

		void Destroy()
		{
			if (dialog != nullptr)
			{
				dialog->StopProgressDialog();
				dialog->Release();
				dialog = nullptr;
			}
		}

		void SetStatus(const CString& phase, const CString& detail, const CString& detail2 = CString())
		{
			if (dialog == nullptr)
				return;

			dialog->SetLine(1, phase, FALSE, nullptr);
			dialog->SetLine(2, detail, FALSE, nullptr);
			dialog->SetLine(3, detail2, FALSE, nullptr);
		}

		void SetProgress(const ULONGLONG completed, const ULONGLONG total)
		{
			if (dialog == nullptr)
				return;

			dialog->SetProgress64(completed, max<ULONGLONG>(1, total));
		}

		bool HasUserCancelled() const
		{
			return dialog != nullptr && dialog->HasUserCancelled() != FALSE;
		}

	private:
		IProgressDialog* dialog = nullptr;
	};

	// The UI thread keeps pumping messages while it waits so the dialog remains responsive.
	void PumpPendingMessages()
	{
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	void UpdateProgressState(ProgressState& state, const CString& phase, const CString& detail, const ULONGLONG completed, const ULONGLONG total, const CString& detail2 = CString())
	{
		// The worker updates all displayed lines atomically so the UI never mixes old and new text.
		std::lock_guard<std::mutex> lock(state.mutex);
		state.phase = phase;
		state.detail = detail;
		state.detail2 = detail2;
		state.completed = completed;
		state.total = max<ULONGLONG>(1, total);
		++state.revision;
	}

	void SyncProgressDialog(ProgressDialog& progress, ProgressState& state, unsigned long& revision)
	{
		// Only push text into the dialog when the worker actually published something new.
		CString phase;
		CString detail;
		CString detail2;
		ULONGLONG completed = 0;
		ULONGLONG total = 1;
		{
			std::lock_guard<std::mutex> lock(state.mutex);
			if (state.revision == revision)
				return;
			phase = state.phase;
			detail = state.detail;
			detail2 = state.detail2;
			completed = state.completed;
			total = state.total;
			revision = state.revision;
		}

		progress.SetStatus(phase, detail, detail2);
		progress.SetProgress(completed, total);
	}

	bool PollProgressCancellation(CancellationContext& cancellation, ProgressDialog& progress)
	{
		// The shell dialog owns the cancel button; once pressed we fan that out to the worker and HTTP layer.
		PumpPendingMessages();
		if (progress.HasUserCancelled())
		{
			cancellation.RequestCancel();
			progress.SetStatus(LoadStringResource(IDS_PROGRESS_CANCEL_PHASE), LoadStringResource(IDS_PROGRESS_CANCEL_DETAIL));
		}

		return cancellation.IsCanceled();
	}

	bool WaitForThrottleDelay(CancellationContext& cancellation, const DWORD delay_ms)
	{
		// Throttle waits must stay cancelable, otherwise the UI would feel stuck for the final sleep.
		if (cancellation.IsCanceled())
			return false;

		const HANDLE cancel_event = cancellation.GetEvent();
		if (cancel_event == nullptr)
			return !cancellation.IsCanceled();

		return ::WaitForSingleObject(cancel_event, delay_ms) == WAIT_TIMEOUT;
	}

	bool WaitForWorkerThread(HANDLE worker_handle, CancellationContext& cancellation, ProgressDialog& progress, ProgressState& progress_state)
	{
		// We wait in short slices so the dialog keeps updating and cancel stays responsive while the worker runs.
		if (worker_handle == nullptr)
			return false;

		HANDLE wait_handles[1] = { worker_handle };
		unsigned long applied_revision = 0;
		for (;;)
		{
			SyncProgressDialog(progress, progress_state, applied_revision);
			if (PollProgressCancellation(cancellation, progress))
				return false;

			const DWORD wait_result = ::MsgWaitForMultipleObjects(1, wait_handles, FALSE, worker_poll_interval_ms, QS_ALLINPUT);
			if (wait_result == WAIT_OBJECT_0)
				break;

			if (wait_result == WAIT_TIMEOUT || wait_result == WAIT_OBJECT_0 + 1)
			{
				PumpPendingMessages();
				continue;
			}

			return false;
		}

		SyncProgressDialog(progress, progress_state, applied_revision);
		PollProgressCancellation(cancellation, progress);
		return true;
	}

	struct GpsNumberToken
	{
		double value = 0.0;
		int start = 0;
		int end = 0;
		WCHAR direction = 0;
		int axis_hint = 0;
	};

	bool IsNumericChar(const WCHAR ch)
	{
		return iswdigit(ch) != 0 || ch == L'+' || ch == L'-' || ch == L'.' || ch == L',';
	}

	bool IsWordLetter(const WCHAR ch)
	{
		return iswalpha(ch) != 0;
	}

	bool IsSkippedDirectionChar(const WCHAR ch)
	{
		return ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n' || wcschr(L":;=/()[]{}<>|,", ch) != nullptr;
	}

	bool IsStandaloneCompassLetter(const CString& input, const int index)
	{
		if (index < 0 || index >= input.GetLength())
			return false;

		const WCHAR upper = static_cast<WCHAR>(towupper(input[index]));
		if (upper != L'N' && upper != L'S' && upper != L'E' && upper != L'W')
			return false;

		if (index > 0 && IsWordLetter(input[index - 1]))
			return false;
		if (index + 1 < input.GetLength() && IsWordLetter(input[index + 1]))
			return false;

		return true;
	}

	WCHAR FindAdjacentDirection(const CString& input, const int start, const int end)
	{
		for (int index = start - 1; index >= 0; --index)
		{
			const WCHAR ch = input[index];
			if (IsSkippedDirectionChar(ch))
				continue;

			if (IsWordLetter(ch))
			{
				if (IsStandaloneCompassLetter(input, index))
					return static_cast<WCHAR>(towupper(ch));
			}

			break;
		}

		for (int index = end; index < input.GetLength(); ++index)
		{
			const WCHAR ch = input[index];
			if (IsSkippedDirectionChar(ch))
				continue;

			if (IsWordLetter(ch))
			{
				if (IsStandaloneCompassLetter(input, index))
					return static_cast<WCHAR>(towupper(ch));
			}

			break;
		}

		return 0;
	}

	int FindAxisHint(const CString& input, const int start, const int end)
	{
		CString lower(input);
		lower.MakeLower();

		const int left_start = max(0, start - 12);
		const int left_length = start - left_start;
		if (left_length > 0)
		{
			const CString context = lower.Mid(left_start, left_length);
			if (context.Find(L"lat") >= 0)
				return 1;
			if (context.Find(L"lon") >= 0 || context.Find(L"lng") >= 0 || context.Find(L"long") >= 0)
				return 2;
		}

		const int right_end = min(lower.GetLength(), end + 12);
		const int right_length = right_end - end;
		if (right_length > 0)
		{
			const CString context = lower.Mid(end, right_length);
			if (context.Find(L"lat") >= 0)
				return 1;
			if (context.Find(L"lon") >= 0 || context.Find(L"lng") >= 0 || context.Find(L"long") >= 0)
				return 2;
		}

		return 0;
	}

	int GetAxisFromToken(const GpsNumberToken& token)
	{
		if (token.direction == L'N' || token.direction == L'S' || token.axis_hint == 1)
			return 1;
		if (token.direction == L'E' || token.direction == L'W' || token.axis_hint == 2)
			return 2;
		return 0;
	}

	double BuildCoordinateFromTokens(const vector<GpsNumberToken>& values, const size_t start_index, size_t& next_index)
	{
		double coordinate = fabs(values[start_index].value);
		next_index = start_index + 1;

		if (next_index < values.size() && GetAxisFromToken(values[next_index]) == 0 && fabs(values[next_index].value) < 60.0)
		{
			coordinate += fabs(values[next_index].value) / 60.0;
			++next_index;

			if (next_index < values.size() && GetAxisFromToken(values[next_index]) == 0 && fabs(values[next_index].value) < 60.0)
			{
				coordinate += fabs(values[next_index].value) / 3600.0;
				++next_index;
			}
		}

		if (values[start_index].value < 0.0 || values[start_index].direction == L'S' || values[start_index].direction == L'W')
			coordinate = -coordinate;

		return coordinate;
	}

	CString FormatCoordinateForUrl(const double value)
	{
		CString text;
		text.Format(L"%.6f", value);
		text.Replace(L',', L'.');
		return text;
	}

	CString Utf8ToCString(const std::string& text)
	{
		if (text.empty())
			return CString();

		const int length = ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
		if (length <= 0)
			return CString();

		CString wide;
		wchar_t* buffer = wide.GetBuffer(length);
		::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), buffer, length);
		wide.ReleaseBuffer(length);
		return wide;
	}

	int HexValue(const WCHAR ch)
	{
		if (ch >= L'0' && ch <= L'9')
			return ch - L'0';
		if (ch >= L'a' && ch <= L'f')
			return 10 + ch - L'a';
		if (ch >= L'A' && ch <= L'F')
			return 10 + ch - L'A';
		return -1;
	}

	bool TryExtractJsonString(const CString& json, const CString& key, CString& value);

	bool TryGetFirstNamedAddressPart(const CString& address_json, const WCHAR* const* keys, const int key_count, CString& value)
	{
		for (int index = 0; index < key_count; ++index)
		{
			if (TryExtractJsonString(address_json, keys[index], value))
			{
				value.Trim();
				if (!value.IsEmpty())
					return true;
			}
		}

		value.Empty();
		return false;
	}

	bool TryExtractJsonObject(const CString& json, const CString& key, CString& object_text)
	{
		const CString needle = L"\"" + key + L"\"";
		const int key_pos = json.Find(needle);
		if (key_pos < 0)
			return false;

		const int colon_pos = json.Find(L':', key_pos + needle.GetLength());
		if (colon_pos < 0)
			return false;

		const int object_start = json.Find(L'{', colon_pos + 1);
		if (object_start < 0)
			return false;

		bool in_string = false;
		bool escape = false;
		int depth = 0;

		for (int index = object_start; index < json.GetLength(); ++index)
		{
			const WCHAR ch = json[index];
			if (in_string)
			{
				if (escape)
					escape = false;
				else if (ch == L'\\')
					escape = true;
				else if (ch == L'\"')
					in_string = false;
				continue;
			}

			if (ch == L'\"')
			{
				in_string = true;
			}
			else if (ch == L'{')
			{
				++depth;
			}
			else if (ch == L'}')
			{
				--depth;
				if (depth == 0)
				{
					object_text = json.Mid(object_start, index - object_start + 1);
					return true;
				}
			}
		}

		return false;
	}

	bool TryExtractJsonString(const CString& json, const CString& key, CString& value)
	{
		const CString needle = L"\"" + key + L"\"";
		const int key_pos = json.Find(needle);
		if (key_pos < 0)
			return false;

		const int colon_pos = json.Find(L':', key_pos + needle.GetLength());
		if (colon_pos < 0)
			return false;

		int value_pos = colon_pos + 1;
		while (value_pos < json.GetLength() && json[value_pos] <= L' ')
			++value_pos;

		if (value_pos >= json.GetLength() || json[value_pos] != L'\"')
			return false;

		CString result;
		bool escape = false;

		for (int index = value_pos + 1; index < json.GetLength(); ++index)
		{
			const WCHAR ch = json[index];
			if (escape)
			{
				switch (ch)
				{
				case L'\"':
				case L'\\':
				case L'/':
					result += ch;
					break;
				case L'b':
					result += L'\b';
					break;
				case L'f':
					result += L'\f';
					break;
				case L'n':
					result += L'\n';
					break;
				case L'r':
					result += L'\r';
					break;
				case L't':
					result += L'\t';
					break;
				case L'u':
					if (index + 4 < json.GetLength())
					{
						int unicode_value = 0;
						bool valid = true;
						for (int hex_index = 0; hex_index < 4; ++hex_index)
						{
							const int hex_value = HexValue(json[index + 1 + hex_index]);
							if (hex_value < 0)
							{
								valid = false;
								break;
							}
							unicode_value = unicode_value * 16 + hex_value;
						}
						if (valid)
						{
							result += static_cast<WCHAR>(unicode_value);
							index += 4;
						}
					}
					break;
				default:
					result += ch;
					break;
				}

				escape = false;
				continue;
			}

			if (ch == L'\\')
			{
				escape = true;
			}
			else if (ch == L'\"')
			{
				value = result;
				return true;
			}
			else
			{
				result += ch;
			}
		}

		return false;
	}

	CString TrimDisplayName(CString text)
	{
		const int comma = text.Find(L',');
		if (comma > 0)
			text = text.Left(comma);
		text.Trim();
		return text;
	}

	bool SameTextInsensitive(const CString& lhs, const CString& rhs)
	{
		return lhs.CompareNoCase(rhs) == 0;
	}

	CString JoinLocationParts(const CString& first, const CString& second)
	{
		if (first.IsEmpty())
			return second;
		if (second.IsEmpty())
			return first;
		if (SameTextInsensitive(first, second))
			return first;
		return first + L", " + second;
	}

	CString PickLocationNameFromNominatimJson(const CString& json)
	{
		if (json.Find(L"\"error\"") >= 0)
			return CString();

		CString address_json;
		if (TryExtractJsonObject(json, L"address", address_json))
		{
			static const std::array<const WCHAR*, 5> road_keys = { L"road", L"pedestrian", L"residential", L"footway", L"path" };
			static const std::array<const WCHAR*, 4> neighbourhood_keys = { L"neighbourhood", L"quarter", L"city_district", L"hamlet" };
			static const std::array<const WCHAR*, 2> suburb_keys = { L"suburb", L"municipality" };
			static const std::array<const WCHAR*, 4> city_keys = { L"city", L"town", L"village", L"state" };
			static const std::array<const WCHAR*, 1> county_keys = { L"county" };
			static const std::array<const WCHAR*, 1> house_number_keys = { L"house_number" };
			static const std::array<const WCHAR*, 1> borough_keys = { L"borough" };

			CString house_number;
			CString road;
			CString neighbourhood;
			CString suburb;
			CString borough;
			CString city;
			CString county;

			TryGetFirstNamedAddressPart(address_json, house_number_keys.data(), static_cast<int>(house_number_keys.size()), house_number);
			TryGetFirstNamedAddressPart(address_json, road_keys.data(), static_cast<int>(road_keys.size()), road);

			if (!road.IsEmpty() && !house_number.IsEmpty())
				road += L" " + house_number;

			TryGetFirstNamedAddressPart(address_json, neighbourhood_keys.data(), static_cast<int>(neighbourhood_keys.size()), neighbourhood);
			TryGetFirstNamedAddressPart(address_json, suburb_keys.data(), static_cast<int>(suburb_keys.size()), suburb);
			TryGetFirstNamedAddressPart(address_json, borough_keys.data(), static_cast<int>(borough_keys.size()), borough);
			TryGetFirstNamedAddressPart(address_json, city_keys.data(), static_cast<int>(city_keys.size()), city);
			TryGetFirstNamedAddressPart(address_json, county_keys.data(), static_cast<int>(county_keys.size()), county);

			if (!neighbourhood.IsEmpty())
				return JoinLocationParts(neighbourhood, city);
			if (!suburb.IsEmpty())
				return JoinLocationParts(suburb, city);
			if (!borough.IsEmpty())
				return JoinLocationParts(borough, city);
			if (!city.IsEmpty())
				return city;
			if (!road.IsEmpty())
				return JoinLocationParts(road, city);
			if (!county.IsEmpty())
				return county;
		}

		CString fallback;
		if (TryExtractJsonString(json, L"name", fallback))
		{
			fallback.Trim();
			if (!fallback.IsEmpty())
				return fallback;
		}

		if (TryExtractJsonString(json, L"display_name", fallback))
			return TrimDisplayName(fallback);

		return CString();
	}

	bool HasExplicitGpsDirections(const CString& gps_text)
	{
		for (int index = 0; index < gps_text.GetLength(); ++index)
		{
			if (IsStandaloneCompassLetter(gps_text, index))
				return true;
		}

		return false;
	}

	// The actual reverse-geocoding request is synchronous, but cancellation can still interrupt it by closing the request handle.
	bool TryFetchNominatimLocationName(const GeoPoint& geo, CString& place_name, CancellationContext* cancellation)
	{
		place_name.Empty();
		if (cancellation != nullptr && cancellation->IsCanceled())
			return false;

		WinHttpHandle session(::WinHttpOpen(
			L"TimeCapsule/1.0",
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0));
		if (!session)
			return false;
		if (cancellation != nullptr && cancellation->IsCanceled())
			return false;

		::WinHttpSetTimeouts(session, 1500, 1500, 2500, 2500);

		WinHttpHandle connection(::WinHttpConnect(session, L"nominatim.openstreetmap.org", INTERNET_DEFAULT_HTTPS_PORT, 0));
		if (!connection)
			return false;
		if (cancellation != nullptr && cancellation->IsCanceled())
			return false;

		CString request_path;
		request_path.Format(
			L"/reverse?format=jsonv2&lat=%s&lon=%s&zoom=18&addressdetails=1&accept-language=de,en",
			FormatCoordinateForUrl(geo.latitude).GetString(),
			FormatCoordinateForUrl(geo.longitude).GetString());

		WinHttpHandle request(::WinHttpOpenRequest(
			connection,
			L"GET",
			request_path,
			nullptr,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			WINHTTP_FLAG_SECURE));
		if (!request)
			return false;

		ActiveRequestScope active_request(cancellation, request);
		if (!active_request.IsRegistered())
			return false;

		const WCHAR* headers = L"Accept: application/json\r\n";
		bool ok = ::WinHttpSendRequest(request, headers, static_cast<DWORD>(-1L), nullptr, 0, 0, 0) == TRUE;
		if (ok)
			ok = ::WinHttpReceiveResponse(request, nullptr) == TRUE;
		if (cancellation != nullptr && cancellation->IsCanceled())
			return false;

		DWORD status_code = 0;
		DWORD status_size = sizeof(status_code);
		if (ok)
		{
			ok = ::WinHttpQueryHeaders(
				request,
				WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
				WINHTTP_HEADER_NAME_BY_INDEX,
				&status_code,
				&status_size,
				WINHTTP_NO_HEADER_INDEX) == TRUE;
		}

		std::string response_utf8;
		while (ok)
		{
			if (cancellation != nullptr && cancellation->IsCanceled())
				return false;

			DWORD available = 0;
			if (::WinHttpQueryDataAvailable(request, &available) != TRUE)
				break;

			if (available == 0)
				break;

			std::string chunk;
			chunk.resize(available);

			DWORD downloaded = 0;
			if (::WinHttpReadData(request, &chunk[0], available, &downloaded) != TRUE)
				break;

			chunk.resize(downloaded);
			response_utf8 += chunk;
		}
		if (cancellation != nullptr && cancellation->IsCanceled())
			return false;

		if (!ok || status_code != 200 || response_utf8.empty())
			return false;

		place_name = PickLocationNameFromNominatimJson(Utf8ToCString(response_utf8));
		place_name.Trim();
		return !place_name.IsEmpty();
	}

	// Some EXIF variants swap latitude and longitude, so we retry once with swapped coordinates when that is plausible.
	bool TryResolveLocationNameNominatim(const CString& gps_text, const GeoPoint& geo, CString& place_name, CancellationContext* cancellation)
	{
		if (TryFetchNominatimLocationName(geo, place_name, cancellation))
			return true;
		if (cancellation != nullptr && cancellation->IsCanceled())
			return false;

		if (HasExplicitGpsDirections(gps_text))
			return false;

		GeoPoint swapped_geo;
		swapped_geo.valid = true;
		swapped_geo.latitude = geo.longitude;
		swapped_geo.longitude = geo.latitude;

		if (fabs(swapped_geo.latitude) > 90.0 || fabs(swapped_geo.longitude) > 180.0)
			return false;

		return TryFetchNominatimLocationName(swapped_geo, place_name, cancellation);
	}

	void AppendGpsNumberToken(const CString& input, CString& current, int& token_start, const int token_end, vector<GpsNumberToken>& values)
	{
		if (current.IsEmpty())
			return;

		current.Replace(L',', L'.');

		GpsNumberToken token;
		token.value = _wtof(current);
		token.start = token_start;
		token.end = token_end;
		token.direction = FindAdjacentDirection(input, token.start, token.end);
		token.axis_hint = FindAxisHint(input, token.start, token.end);
		values.emplace_back(token);

		current.Empty();
		token_start = -1;
	}

	// GPS strings come back in different textual variants, so we first extract the numeric parts.
	bool ExtractNumbers(const CString& input, vector<GpsNumberToken>& values)
	{
		CString current;
		int token_start = -1;
		for (int index = 0; index < input.GetLength(); ++index)
		{
			const WCHAR ch = input[index];
			if (IsNumericChar(ch))
			{
				if (current.IsEmpty())
					token_start = index;
				current += ch;
			}
			else if (!current.IsEmpty())
			{
				AppendGpsNumberToken(input, current, token_start, index, values);
			}
		}

		AppendGpsNumberToken(input, current, token_start, input.GetLength(), values);

		return values.size() >= 2;
	}

	// EXIF GPS may arrive either as decimal degrees or as degree/minute/second sequences.
	bool TryParseDecimalGps(const CString& gps_text, GeoPoint& point)
	{
		vector<GpsNumberToken> values;
		if (!ExtractNumbers(gps_text, values))
			return false;

		double latitude = 0.0;
		double longitude = 0.0;
		bool have_latitude = false;
		bool have_longitude = false;

		for (size_t index = 0; index < values.size(); ++index)
		{
			const int axis = GetAxisFromToken(values[index]);
			if (axis == 0)
				continue;

			size_t next_index = index + 1;
			const double value = BuildCoordinateFromTokens(values, index, next_index);

			if (axis == 1)
			{
				latitude = value;
				have_latitude = true;
			}
			else if (axis == 2)
			{
				longitude = value;
				have_longitude = true;
			}

			index = next_index - 1;
		}

		if (!have_latitude || !have_longitude)
		{
			double first = values[0].value;
			double second = values[1].value;

			if (!have_latitude && !have_longitude)
			{
				const bool first_can_be_lat = fabs(first) <= 90.0;
				const bool first_can_be_lon = fabs(first) <= 180.0;
				const bool second_can_be_lat = fabs(second) <= 90.0;
				const bool second_can_be_lon = fabs(second) <= 180.0;

				if (!first_can_be_lat && second_can_be_lat)
				{
					latitude = second;
					longitude = first;
				}
				else if (!second_can_be_lat && first_can_be_lat)
				{
					latitude = first;
					longitude = second;
				}
				else
				{
					latitude = first;
					longitude = second;
				}
				have_latitude = true;
				have_longitude = true;
			}
			else if (!have_latitude && have_longitude)
			{
				latitude = fabs(first - longitude) < fabs(second - longitude) ? second : first;
				have_latitude = true;
			}
			else if (have_latitude && !have_longitude)
			{
				longitude = fabs(first - latitude) < fabs(second - latitude) ? second : first;
				have_longitude = true;
			}
		}

		CString upper(gps_text);
		upper.MakeUpper();
		if (!have_latitude && upper.Find(L'S') >= 0)
			latitude = -fabs(latitude);
		if (!have_longitude && upper.Find(L'W') >= 0)
			longitude = -fabs(longitude);

		if (fabs(latitude) > 90.0 || fabs(longitude) > 180.0)
			return false;

		point.valid = true;
		point.latitude = latitude;
		point.longitude = longitude;
		return true;
	}

	double DegreesToRadians(const double degrees)
	{
		return degrees * 3.14159265358979323846 / 180.0;
	}

	double HaversineKm(const GeoPoint& lhs, const GeoPoint& rhs)
	{
		const double lat1 = DegreesToRadians(lhs.latitude);
		const double lat2 = DegreesToRadians(rhs.latitude);
		const double dlat = lat2 - lat1;
		const double dlon = DegreesToRadians(rhs.longitude - lhs.longitude);

		const double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
			cos(lat1) * cos(lat2) * sin(dlon / 2.0) * sin(dlon / 2.0);
		const double c = 2.0 * atan2(sqrt(a), sqrt(max(0.0, 1.0 - a)));
		return 6371.0 * c;
	}

	CString FormatDistance(const double km)
	{
		CString format;
		if (km < 10.0)
			format.LoadString(IDS_DISTANCE_DECIMAL_KM);
		else
			format.LoadString(IDS_DISTANCE_INTEGER_KM);

		CString text;
		text.Format(format, km);
		return text;
	}

	CString FormatDuration(__int64 seconds)
	{
		if (seconds <= 0)
		{
			CString text;
			text.LoadString(IDS_DURATION_ZERO_HOURS);
			return text;
		}

		const __int64 days = seconds / 86400;
		const __int64 hours = (seconds % 86400) / 3600;
		CString format;
		if (days > 0)
		{
			format.LoadString(IDS_DURATION_DAYS_HOURS);
			CString text;
			text.Format(format, days, hours);
			return text;
		}

		format.LoadString(IDS_DURATION_HOURS);
		CString text;
		text.Format(format, max<__int64>(1, hours));
		return text;
	}

	CString FormatLocationName(const int cluster_index)
	{
		CString format;
		format.LoadString(IDS_LOCATION_FORMAT);
		CString text;
		text.Format(format, cluster_index + 1);
		return text;
	}

	CString GetClusterName(const RouteCluster& cluster, const int cluster_index)
	{
		if (!cluster.place_name.IsEmpty())
			return cluster.place_name;

		return FormatLocationName(cluster_index);
	}

	CString FormatLocationLabel(const RouteCluster& cluster, const int cluster_index)
	{
		if (!cluster.place_name.IsEmpty())
			return cluster.place_name;

		CString latitude_text;
		latitude_text.Format(L"%.4f", cluster.latitude);
		CString longitude_text;
		longitude_text.Format(L"%.4f", cluster.longitude);

		CString text;
		text.Format(L"%s (%s, %s)", GetClusterName(cluster, cluster_index).GetString(),
			latitude_text.GetString(),
			longitude_text.GetString());
		return text;
	}

	CString ShortGpsText(const CString& gps)
	{
		CString text(gps);
		text.Trim();
		if (text.GetLength() > 28)
			text = text.Left(28) + L"...";
		return text;
	}

	CString GetSortModeText(const int sort_mode)
	{
		CString text;
		switch (sort_mode)
		{
		case SORT_MODE_SELECTION:
			text.LoadString(IDS_SORT_MODE_SELECTION);
			return text;
		case SORT_MODE_GPS_WEST_EAST:
			text.LoadString(IDS_SORT_MODE_WEST_EAST);
			return text;
		case SORT_MODE_GPS_NORTH_SOUTH:
			text.LoadString(IDS_SORT_MODE_NORTH_SOUTH);
			return text;
		case SORT_MODE_GPS_EAST_WEST:
			text.LoadString(IDS_SORT_MODE_EAST_WEST);
			return text;
		case SORT_MODE_GPS_SOUTH_NORTH:
			text.LoadString(IDS_SORT_MODE_SOUTH_NORTH);
			return text;
		case SORT_MODE_EXIF:
		default:
			text.LoadString(IDS_SORT_MODE_EXIF);
			return text;
		}
	}

	// Nearby points are grouped into route stops so the poster does not show every GPS jitter as a new place.
	int FindOrCreateCluster(const GeoPoint& geo, vector<RouteCluster>& clusters, const double cluster_radius_km)
	{
		for (size_t index = 0; index < clusters.size(); ++index)
		{
			GeoPoint center;
			center.valid = true;
			center.latitude = clusters[index].latitude;
			center.longitude = clusters[index].longitude;
			if (HaversineKm(geo, center) <= cluster_radius_km)
			{
				const double weight = static_cast<double>(clusters[index].count);
				clusters[index].latitude = (clusters[index].latitude * weight + geo.latitude) / (weight + 1.0);
				clusters[index].longitude = (clusters[index].longitude * weight + geo.longitude) / (weight + 1.0);
				++clusters[index].count;
				return static_cast<int>(index);
			}
		}

		RouteCluster cluster;
		cluster.latitude = geo.latitude;
		cluster.longitude = geo.longitude;
		cluster.count = 1;
		cluster.color = GetClusterColor(static_cast<int>(clusters.size()));
		clusters.emplace_back(cluster);
		return static_cast<int>(clusters.size() - 1);
	}

	// Build the travel summary on the UI thread before the worker starts so clustering data is complete and stable.
	bool AnalyzePictures(
		const vector<const picture_data*>& pictures,
		const double cluster_radius_km,
		PosterInsights& insights,
		CancellationContext* cancellation,
		ProgressDialog* progress)
	{
		insights = PosterInsights();
		insights.entries.reserve(pictures.size());

		bool have_prev_geo = false;
		GeoPoint prev_geo;

		for (size_t picture_index = 0; picture_index < pictures.size(); ++picture_index)
		{
			if (cancellation != nullptr && cancellation->IsCanceled())
				return false;

			if (progress != nullptr && (picture_index == 0 || picture_index + 1 == pictures.size() || (picture_index % 16) == 0))
			{
				// Analysis is cheap, so sparse updates are enough to show life without spamming the dialog.
				progress->SetStatus(
					LoadStringResource(IDS_PROGRESS_ANALYZE_PHASE),
					FormatStringResource(IDS_PROGRESS_ANALYZE_DETAIL, static_cast<int>(picture_index + 1), static_cast<int>(pictures.size())));
				progress->SetProgress(static_cast<ULONGLONG>(picture_index + 1), static_cast<ULONGLONG>(max<size_t>(1, pictures.size())));
				if (cancellation != nullptr && PollProgressCancellation(*cancellation, *progress))
					return false;
			}

			const picture_data* picture = pictures[picture_index];
			PosterEntry entry;
			entry.picture = picture;
			entry.geo.valid = false;

			if (TryParseDecimalGps(picture->gps, entry.geo))
			{
				entry.cluster_index = FindOrCreateCluster(entry.geo, insights.clusters, cluster_radius_km);
				++insights.gps_picture_count;

				if (have_prev_geo)
				{
					entry.hop_distance_km = HaversineKm(prev_geo, entry.geo);
					insights.total_distance_km += entry.hop_distance_km;
					if (entry.hop_distance_km > insights.longest_hop_km)
					{
						insights.longest_hop_km = entry.hop_distance_km;
						insights.longest_hop_index = static_cast<int>(insights.entries.size());
					}
				}

				prev_geo = entry.geo;
				have_prev_geo = true;
			}

			if (picture->exif_time != 0)
			{
				if (insights.first_exif_time == 0 || picture->exif_time < insights.first_exif_time)
					insights.first_exif_time = picture->exif_time;
				if (insights.last_exif_time == 0 || picture->exif_time > insights.last_exif_time)
					insights.last_exif_time = picture->exif_time;
			}

			insights.entries.emplace_back(entry);
		}

		return true;
	}

	bool TryGetClusterGpsText(const vector<PosterEntry>& entries, const int cluster_index, CString& gps_text)
	{
		// Each cluster borrows the first original GPS string so the fallback coordinate logic sees the same EXIF text.
		for (const PosterEntry& entry : entries)
		{
			if (entry.cluster_index == cluster_index && entry.picture != nullptr)
			{
				gps_text = entry.picture->gps;
				return true;
			}
		}

		gps_text.Empty();
		return false;
	}

	// Reverse geocoding runs in the worker so the UI thread can keep the dialog responsive and cancelable.
	bool ResolveClusterNames(
		vector<RouteCluster>& clusters,
		const vector<PosterEntry>& entries,
		CancellationContext& cancellation,
		ProgressState& progress_state)
	{
		const int cluster_count = static_cast<int>(clusters.size());
		for (size_t index = 0; index < clusters.size(); ++index)
		{
			if (cancellation.IsCanceled())
				return false;

			CString gps_text;
			TryGetClusterGpsText(entries, static_cast<int>(index), gps_text);

			CString detail;
			if (!gps_text.IsEmpty())
				detail = FormatStringResource(IDS_PROGRESS_LOOKUP_DETAIL_GPS, static_cast<int>(index + 1), cluster_count, ShortGpsText(gps_text).GetString());
			else
				detail = FormatStringResource(IDS_PROGRESS_LOOKUP_DETAIL, static_cast<int>(index + 1), cluster_count);
			UpdateProgressState(
				progress_state,
				LoadStringResource(IDS_PROGRESS_LOOKUP_PHASE),
				detail,
				static_cast<ULONGLONG>(index),
				static_cast<ULONGLONG>(max(1, cluster_count)));

			GeoPoint center;
			center.valid = true;
			center.latitude = clusters[index].latitude;
			center.longitude = clusters[index].longitude;

			CString place_name;
			if (TryResolveLocationNameNominatim(gps_text, center, place_name, &cancellation))
				clusters[index].place_name = place_name;

			CString resolved_detail;
			if (!place_name.IsEmpty())
				resolved_detail = FormatStringResource(IDS_PROGRESS_LOOKUP_DETAIL_NAME, static_cast<int>(index + 1), cluster_count, place_name.GetString());
			UpdateProgressState(
				progress_state,
				LoadStringResource(IDS_PROGRESS_LOOKUP_PHASE),
				detail,
				static_cast<ULONGLONG>(index + 1),
				static_cast<ULONGLONG>(max(1, cluster_count)),
				resolved_detail);

			if (cancellation.IsCanceled())
				return false;

			if (index + 1 < clusters.size())
			{
				// Public Nominatim usage is limited to at most one request per second, so we pause before every next lookup.
				CString throttle_detail = FormatStringResource(IDS_PROGRESS_THROTTLE_DETAIL, static_cast<int>(index + 2), cluster_count);
				CString throttle_detail2;
				if (!place_name.IsEmpty())
					throttle_detail2 = FormatStringResource(IDS_PROGRESS_LOOKUP_DETAIL_NAME, static_cast<int>(index + 1), cluster_count, place_name.GetString());
				UpdateProgressState(
					progress_state,
					LoadStringResource(IDS_PROGRESS_THROTTLE_PHASE),
					throttle_detail,
					static_cast<ULONGLONG>(index + 1),
					static_cast<ULONGLONG>(max(1, cluster_count)),
					throttle_detail2);
				if (!WaitForThrottleDelay(cancellation, throttled_lookup_delay_ms))
					return false;
			}
		}

		UpdateProgressState(progress_state, LoadStringResource(IDS_PROGRESS_RENDER_PHASE), CString(), static_cast<ULONGLONG>(cluster_count), static_cast<ULONGLONG>(max(1, cluster_count)));
		return true;
	}

	// The worker only needs immutable input plus a mutable cluster list and the shared progress/cancel state.
	struct ClusterLookupWorkerContext
	{
		vector<RouteCluster>* clusters = nullptr;
		const vector<PosterEntry>* entries = nullptr;
		CancellationContext* cancellation = nullptr;
		ProgressState* progress_state = nullptr;
		bool completed = false;
	};

	unsigned __stdcall ResolveClusterNamesWorkerProc(void* raw_context)
	{
		// Keep the thread proc tiny so the actual lookup logic stays testable and readable.
		ClusterLookupWorkerContext* context = static_cast<ClusterLookupWorkerContext*>(raw_context);
		if (context == nullptr || context->clusters == nullptr || context->entries == nullptr || context->cancellation == nullptr || context->progress_state == nullptr)
			return 0;

		context->completed = ResolveClusterNames(*context->clusters, *context->entries, *context->cancellation, *context->progress_state);
		return 0;
	}

	CString BuildSummary(const vector<const picture_data*>& pictures, const PosterInsights& insights)
	{
		CString format;
		format.LoadString(IDS_SUMMARY_PICTURES);
		CString summary;
		summary.Format(format, static_cast<int>(pictures.size()));

		std::map<CString, int> camera_count;

		for (const picture_data* picture : pictures)
		{
			CString model(picture->model);
			model.Trim();
			if (!model.IsEmpty())
				camera_count[model]++;
		}

		format.LoadString(IDS_SUMMARY_EXTRAS);
		CString extras;
		extras.Format(format, insights.gps_picture_count,
			static_cast<int>(insights.clusters.size()), static_cast<int>(camera_count.size()));
		summary += extras;

		if (insights.total_distance_km > 0.0)
		{
			CString prefix;
			prefix.LoadString(IDS_SUMMARY_DISTANCE_PREFIX);
			summary += prefix;
			summary += FormatDistance(insights.total_distance_km);
		}

		if (insights.first_exif_time != 0 && insights.last_exif_time >= insights.first_exif_time)
		{
			CString prefix;
			prefix.LoadString(IDS_SUMMARY_DURATION_PREFIX);
			summary += prefix;
			summary += FormatDuration(insights.last_exif_time - insights.first_exif_time);
		}

		return summary;
	}

		CString BuildStoryLine(const PosterInsights& insights)
		{
			CString story;
			if (!insights.clusters.empty())
			{
				story.LoadString(IDS_STORY_START_PREFIX);
				for (size_t index = 0; index < insights.entries.size(); ++index)
				{
					if (insights.entries[index].cluster_index >= 0)
					{
						story += FormatLocationLabel(insights.clusters[insights.entries[index].cluster_index], insights.entries[index].cluster_index);
						break;
					}
				}

				for (size_t index = insights.entries.size(); index > 0; --index)
				{
					if (insights.entries[index - 1].cluster_index >= 0)
					{
						CString prefix;
						prefix.LoadString(IDS_STORY_DESTINATION_PREFIX);
						story += prefix;
						story += FormatLocationLabel(insights.clusters[insights.entries[index - 1].cluster_index], insights.entries[index - 1].cluster_index);
						break;
					}
				}
			}

			if (insights.longest_hop_km > 0.0)
			{
				if (!story.IsEmpty())
				{
					CString separator;
					separator.LoadString(IDS_STORY_SEPARATOR);
					story += separator;
				}
				CString prefix;
				prefix.LoadString(IDS_STORY_LONGEST_HOP_PREFIX);
				story += prefix;
				story += FormatDistance(insights.longest_hop_km);
			}

			return story;
		}

	CString BuildCaption(const picture_data& picture, bool show_metadata, const PosterEntry& entry, const vector<RouteCluster>& clusters)
	{
		CString line1(GetBaseName(picture.file_name));
		CString line2;
		CString line3;

		if (!picture.exif_datetime_display.IsEmpty())
			line2 = picture.exif_datetime_display;
		else
			line2.Format(L"%d x %d", picture.picture_width, picture.picture_height);

		if (show_metadata)
		{
			CString location;
			if (entry.cluster_index >= 0 && entry.cluster_index < static_cast<int>(clusters.size()))
				location = GetClusterName(clusters[entry.cluster_index], entry.cluster_index);

			CString model(picture.model);
			model.Trim();
			CString lens(picture.lens);
			lens.Trim();

			if (!location.IsEmpty() && !model.IsEmpty())
				line3 = location + L" | " + model;
			else if (!location.IsEmpty())
				line3 = location;
			else if (!model.IsEmpty() && !lens.IsEmpty())
				line3 = model + L" | " + lens;
			else if (!model.IsEmpty())
				line3 = model;
			else if (!picture.gps.IsEmpty())
				line3 = ShortGpsText(picture.gps);
		}

		CString caption(line1 + L"\n" + line2);
		if (!line3.IsEmpty())
			caption += L"\n" + line3;

		return caption;
	}

	bool ComparePicturesByExif(const picture_data* lhs, const picture_data* rhs)
	{
		const bool lhs_has_time = lhs->exif_time != 0;
		const bool rhs_has_time = rhs->exif_time != 0;

		if (lhs_has_time != rhs_has_time)
			return lhs_has_time > rhs_has_time;

		if (lhs->exif_time != rhs->exif_time)
			return lhs->exif_time < rhs->exif_time;

		return lhs->file_name.CompareNoCase(rhs->file_name) < 0;
	}

	bool ComparePicturesByGps(const picture_data* lhs, const picture_data* rhs, const bool use_longitude, const bool ascending)
	{
		GeoPoint lhs_geo;
		GeoPoint rhs_geo;
		const bool lhs_has_geo = TryParseDecimalGps(lhs->gps, lhs_geo);
		const bool rhs_has_geo = TryParseDecimalGps(rhs->gps, rhs_geo);

		if (lhs_has_geo != rhs_has_geo)
			return lhs_has_geo > rhs_has_geo;

		if (lhs_has_geo && rhs_has_geo)
		{
			const double lhs_axis = use_longitude ? lhs_geo.longitude : lhs_geo.latitude;
			const double rhs_axis = use_longitude ? rhs_geo.longitude : rhs_geo.latitude;
			if (lhs_axis != rhs_axis)
				return ascending ? (lhs_axis < rhs_axis) : (lhs_axis > rhs_axis);

			const double lhs_secondary = use_longitude ? lhs_geo.latitude : lhs_geo.longitude;
			const double rhs_secondary = use_longitude ? rhs_geo.latitude : rhs_geo.longitude;
			if (lhs_secondary != rhs_secondary)
				return ascending ? (lhs_secondary < rhs_secondary) : (lhs_secondary > rhs_secondary);
		}

		return ComparePicturesByExif(lhs, rhs);
	}

	// Non-GPS sort modes still fall back to a stable EXIF/name order for deterministic output.
	void SortPictures(vector<const picture_data*>& pictures, const int sort_mode)
	{
		switch (sort_mode)
		{
		case SORT_MODE_GPS_WEST_EAST:
			std::stable_sort(pictures.begin(), pictures.end(),
				[](const picture_data* lhs, const picture_data* rhs) { return ComparePicturesByGps(lhs, rhs, true, true); });
			break;
		case SORT_MODE_GPS_NORTH_SOUTH:
			std::stable_sort(pictures.begin(), pictures.end(),
				[](const picture_data* lhs, const picture_data* rhs) { return ComparePicturesByGps(lhs, rhs, false, false); });
			break;
		case SORT_MODE_GPS_EAST_WEST:
			std::stable_sort(pictures.begin(), pictures.end(),
				[](const picture_data* lhs, const picture_data* rhs) { return ComparePicturesByGps(lhs, rhs, true, false); });
			break;
		case SORT_MODE_GPS_SOUTH_NORTH:
			std::stable_sort(pictures.begin(), pictures.end(),
				[](const picture_data* lhs, const picture_data* rhs) { return ComparePicturesByGps(lhs, rhs, false, true); });
			break;
		case SORT_MODE_EXIF:
			std::stable_sort(pictures.begin(), pictures.end(), ComparePicturesByExif);
			break;
		case SORT_MODE_SELECTION:
		default:
			break;
		}
	}

	void DrawNumberBadge(CDC& mem_dc, const CRect& rect, const COLORREF color, const int sequence_number)
	{
		CFont badge_font;
		badge_font.CreateFont(14, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
			OUT_DEVICE_PRECIS, CLIP_CHARACTER_PRECIS, PROOF_QUALITY,
			VARIABLE_PITCH | FF_SWISS, L"Segoe UI");
		CFont* old_font = mem_dc.SelectObject(&badge_font);
		CBrush brush(color);
		CBrush* old_brush = mem_dc.SelectObject(&brush);
		CPen pen(PS_SOLID, 1, RGB(255, 255, 255));
		CPen* old_pen = mem_dc.SelectObject(&pen);
		mem_dc.Ellipse(rect);
		CString label;
		label.Format(L"%d", sequence_number);
		mem_dc.SetBkMode(TRANSPARENT);
		mem_dc.SetTextColor(RGB(255, 255, 255));
		CRect text_rect(rect);
		mem_dc.DrawText(label, &text_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		mem_dc.SelectObject(old_pen);
		mem_dc.SelectObject(old_brush);
		mem_dc.SelectObject(old_font);
	}

	// The route strip is intentionally schematic: a horizontal travel line reads better than a literal map trace.
	void DrawRouteStrip(CDC& mem_dc, const CRect& route_rect, const PosterInsights& insights)
	{
		if (insights.gps_picture_count > 0)
		{
			CBrush route_background(RGB(49, 63, 78));
			mem_dc.FillRect(route_rect, &route_background);
		}

		if (insights.gps_picture_count < 2)
			return;

		const int left = route_rect.left + 14;
		const int top = route_rect.top + 14;
		const int width = max(1, route_rect.Width() - 28);
		const int height = max(1, route_rect.Height() - 28);
		const int baseline_y = top + (height * 3) / 4;

		vector<CPoint> route_points;
		route_points.reserve(insights.gps_picture_count);

		int valid_index = 0;
		const int valid_count = max(1, insights.gps_picture_count);
		for (size_t index = 0; index < insights.entries.size(); ++index)
		{
			if (!insights.entries[index].geo.valid)
				continue;

			const int x = valid_count == 1
				? left + width / 2
				: left + static_cast<int>((static_cast<double>(valid_index) / (valid_count - 1)) * width);
			route_points.emplace_back(x, baseline_y);
			++valid_index;
		}

		CPen route_pen(PS_SOLID, 3, RGB(225, 214, 170));
		CPen* old_pen = mem_dc.SelectObject(&route_pen);
		bool first = true;

		for (size_t index = 0; index < route_points.size(); ++index)
		{
			const int x = route_points[index].x;
			const int y = route_points[index].y;

			if (first)
			{
				mem_dc.MoveTo(x, y);
				first = false;
			}
			else
			{
				mem_dc.LineTo(x, y);
			}
		}

		mem_dc.SelectObject(old_pen);

		valid_index = 0;
		for (size_t index = 0; index < insights.entries.size(); ++index)
		{
			if (!insights.entries[index].geo.valid || insights.entries[index].cluster_index < 0)
				continue;

			const int x = route_points[valid_index].x;
			const int y = route_points[valid_index].y;
			const int badge_radius = 10;
			const CRect badge_rect(x - badge_radius, y - badge_radius, x + badge_radius + 1, y + badge_radius + 1);
			DrawNumberBadge(mem_dc, badge_rect, insights.clusters[insights.entries[index].cluster_index].color, static_cast<int>(index) + 1);
			++valid_index;
		}

		mem_dc.SetBkMode(TRANSPARENT);
		mem_dc.SetTextColor(RGB(236, 234, 228));
		CRect legend_rect(route_rect.left + 12, route_rect.top + 8, route_rect.right - 12, route_rect.top + 24);
		mem_dc.DrawText(BuildStoryLine(insights), legend_rect, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
	}

	void DrawRequestedPicture(CDC& mem_dc, const requested_data& request, const CRect& target)
	{
		if (request.data == nullptr)
			return;

		HDRAWDIB draw_dib = DrawDibOpen();
		if (draw_dib == nullptr)
			return;

		BITMAPINFOHEADER bmi_header = { 0 };
		bmi_header.biSize = sizeof(BITMAPINFOHEADER);
		bmi_header.biPlanes = 1;
		bmi_header.biBitCount = 24;
		bmi_header.biCompression = BI_RGB;
		bmi_header.biWidth = request.picture_width;
		bmi_header.biHeight = request.picture_height;

		DrawDibDraw(draw_dib,
			mem_dc.m_hDC,
			target.left,
			target.top,
			target.Width(),
			target.Height(),
			&bmi_header,
			request.data,
			0,
			0,
			request.picture_width,
			request.picture_height,
			0);

		DrawDibClose(draw_dib);
	}
}


const CString __stdcall GetPluginVersion()
{
	return L"1.0";
}

const CString __stdcall GetPluginInterfaceVersion()
{
	return L"1.7";
}

const PLUGIN_TYPE __stdcall GetPluginType()
{
	return PLUGIN_TYPE_FUNCTION;
}

const int __stdcall GetPluginInit()
{
	return 1;
}

lpfnFunctionGetInstanceProc __stdcall GetPluginProc(const int k)
{
	return CFunctionPluginTimeCapsule::GetInstance;
}


CFunctionPluginTimeCapsule::CFunctionPluginTimeCapsule()
	: handle_wnd(nullptr),
	  poster_dib(nullptr),
	  sort_mode(SORT_MODE_EXIF),
	  show_metadata(true),
	  thumbnail_width(240),
	  thumbnail_height(150),
	  location_cluster_radius_km(2.0)
{
}

CFunctionPluginTimeCapsule::~CFunctionPluginTimeCapsule()
{
	if (poster_dib != nullptr)
		::DeleteObject(poster_dib);
}

struct plugin_data __stdcall CFunctionPluginTimeCapsule::get_plugin_data() const
{
	struct plugin_data pluginData;
	pluginData.name.LoadString(IDS_PLUGIN_SHORT_DESC);
	pluginData.desc.LoadString(IDS_PLUGIN_LONG_DESC);
	pluginData.info.LoadString(IDS_PLUGIN_INFO);
	return pluginData;
}

struct arg_count __stdcall CFunctionPluginTimeCapsule::get_arg_count() const
{
	return arg_count(2, -1);
}

enum REQUEST_TYPE __stdcall CFunctionPluginTimeCapsule::start(
	const HWND hwnd,
	const vector<const WCHAR*>& file_list,
	vector<request_data_size>& request_data_sizes)
{
	handle_wnd = hwnd;

	if (file_list.size() < 2)
	{
		CString msg;
		msg.LoadString(IDS_MIN_SELECTION);
		::MessageBox(hwnd, msg, get_plugin_data().name, MB_OK | MB_ICONINFORMATION);
		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;
	}

	CWnd parent;
	parent.Attach(handle_wnd);

	CSettingsDlg settings(&parent);
	settings.title_text = title_text;
	settings.sort_mode = sort_mode;
	settings.show_metadata = show_metadata ? TRUE : FALSE;
	settings.thumbnail_width = thumbnail_width;
	settings.thumbnail_height = thumbnail_height;
	settings.location_cluster_radius_km = location_cluster_radius_km;

	if (settings.DoModal() != IDOK)
	{
		parent.Detach();
		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;
	}

	parent.Detach();

	title_text = settings.title_text;
	sort_mode = settings.sort_mode;
	show_metadata = settings.show_metadata == TRUE;
	thumbnail_width = settings.thumbnail_width;
	thumbnail_height = settings.thumbnail_height;
	location_cluster_radius_km = settings.location_cluster_radius_km;

	request_data_sizes.emplace_back(
		thumbnail_width,
		thumbnail_height,
		DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA);

	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginTimeCapsule::process_picture(const picture_data& picture_data)
{
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginTimeCapsule::end(const vector<picture_data>& picture_data_list)
{
	ScopedWaitCursor wait_cursor;
	update_data_list.clear();
	const CString plugin_name = get_plugin_data().name;

	if (picture_data_list.size() < 2)
		return update_data_list;

	if (poster_dib != nullptr)
	{
		::DeleteObject(poster_dib);
		poster_dib = nullptr;
	}

	// The standard progress dialog needs COM and becomes the single cancel source for the whole operation.
	ScopedComInitializer com_initializer;

	CancellationContext cancellation;
	ProgressDialog progress;
	if (!com_initializer.IsAvailable() || !cancellation.IsValid() || !progress.Create(handle_wnd, plugin_name, LoadStringResource(IDS_PROGRESS_CANCEL_MESSAGE)))
	{
		::MessageBox(handle_wnd, LoadStringResource(IDS_ERROR_PROGRESS_DIALOG), plugin_name, MB_OK | MB_ICONERROR);
		return update_data_list;
	}

	progress.SetStatus(LoadStringResource(IDS_PROGRESS_INITIAL_PHASE), LoadStringResource(IDS_PROGRESS_INITIAL_DETAIL));
	progress.SetProgress(0, max<ULONGLONG>(1, static_cast<ULONGLONG>(picture_data_list.size())));

	// Every early exit uses the same cleanup path so partially built poster state is never leaked.
	auto abort_operation = [&]() -> const vector<update_data>&
	{
		update_data_list.clear();
		if (poster_dib != nullptr)
		{
			::DeleteObject(poster_dib);
			poster_dib = nullptr;
		}
		progress.Destroy();
		return update_data_list;
	};

	// Work on pointers so we can reorder the selected pictures without copying the plugin-owned structs.
	vector<const picture_data*> sorted_pictures;
	sorted_pictures.reserve(picture_data_list.size());
	for (const picture_data& picture : picture_data_list)
		sorted_pictures.emplace_back(&picture);

	SortPictures(sorted_pictures, sort_mode);

	// Step 1: cluster and summarize on the UI thread so the worker can read immutable lookup input.
	PosterInsights insights;
	if (!AnalyzePictures(sorted_pictures, location_cluster_radius_km, insights, &cancellation, &progress))
		return abort_operation();

	// Step 2: the worker resolves place names while the UI thread only mirrors status and cancellation.
	ProgressState progress_state;
	UpdateProgressState(progress_state, LoadStringResource(IDS_PROGRESS_LOOKUP_PHASE), LoadStringResource(IDS_PROGRESS_INITIAL_DETAIL), 0, static_cast<ULONGLONG>(max(1, static_cast<int>(insights.clusters.size()))));

	ClusterLookupWorkerContext worker_context;
	worker_context.clusters = &insights.clusters;
	worker_context.entries = &insights.entries;
	worker_context.cancellation = &cancellation;
	worker_context.progress_state = &progress_state;

	ScopedKernelHandle worker_handle(reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, &ResolveClusterNamesWorkerProc, &worker_context, 0, nullptr)));
	if (!worker_handle)
	{
		::MessageBox(handle_wnd, LoadStringResource(IDS_ERROR_PROGRESS_WORKER), plugin_name, MB_OK | MB_ICONERROR);
		return abort_operation();
	}

	if (!WaitForWorkerThread(worker_handle.Get(), cancellation, progress, progress_state) || !worker_context.completed)
		return abort_operation();

	// Step 3: render only after the cluster names are finalized so captions and route labels stay consistent.
	const int count = static_cast<int>(sorted_pictures.size());
	const int cols = min(4, max(2, count));
	const int rows = (count + cols - 1) / cols;
	const int tile_width = thumbnail_width;
	const int tile_height = thumbnail_height + caption_height;
	const int bitmap_width = tile_padding + cols * (tile_width + tile_padding);
	const int current_header_height = insights.gps_picture_count > 0 ? header_height : compact_header_height;
	const int bitmap_height = current_header_height + summary_height + tile_padding + rows * (tile_height + tile_padding);

	BITMAPINFO bitmap_info = { 0 };
	bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 24;
	bitmap_info.bmiHeader.biCompression = BI_RGB;
	bitmap_info.bmiHeader.biWidth = bitmap_width;
	bitmap_info.bmiHeader.biHeight = bitmap_height;

	void* dib_bits = nullptr;
	poster_dib = ::CreateDIBSection(nullptr, &bitmap_info, DIB_RGB_COLORS, &dib_bits, nullptr, 0);
	if (poster_dib == nullptr || dib_bits == nullptr)
		return abort_operation();

	CBitmap* bitmap = CBitmap::FromHandle(poster_dib);
	CDC mem_dc;
	mem_dc.CreateCompatibleDC(nullptr);
	CBitmap* old_bitmap = mem_dc.SelectObject(bitmap);

	CRect full_rect(0, 0, bitmap_width, bitmap_height);
	CBrush bg_brush(RGB(248, 245, 239));
	mem_dc.FillRect(full_rect, &bg_brush);

	CRect header_rect(0, 0, bitmap_width, current_header_height);
	CBrush header_brush(RGB(34, 49, 63));
	mem_dc.FillRect(header_rect, &header_brush);

	CPen frame_pen(PS_SOLID, 2, RGB(210, 184, 124));
	CPen* old_pen = mem_dc.SelectObject(&frame_pen);

	CFont title_font;
	title_font.CreateFont(34, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
		OUT_DEVICE_PRECIS, CLIP_CHARACTER_PRECIS, PROOF_QUALITY,
		VARIABLE_PITCH | FF_SWISS, L"Segoe UI");
	CFont summary_font;
	summary_font.CreateFont(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_DEVICE_PRECIS, CLIP_CHARACTER_PRECIS, PROOF_QUALITY,
		VARIABLE_PITCH | FF_SWISS, L"Segoe UI");
	CFont caption_font;
	caption_font.CreateFont(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_DEVICE_PRECIS, CLIP_CHARACTER_PRECIS, PROOF_QUALITY,
		VARIABLE_PITCH | FF_SWISS, L"Segoe UI");

	CFont* old_font = mem_dc.SelectObject(&title_font);
	mem_dc.SetBkMode(TRANSPARENT);
	mem_dc.SetTextColor(RGB(255, 248, 233));
	CRect title_rect(24, 16, bitmap_width - 24, 54);
	CString header_title(title_text);
	if (header_title.IsEmpty())
		header_title.LoadString(IDS_DEFAULT_HEADER_TITLE);
	mem_dc.DrawText(header_title, title_rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

	mem_dc.SelectObject(&summary_font);
	mem_dc.SetTextColor(RGB(26, 31, 37));
	if (insights.gps_picture_count > 0)
	{
		CRect route_rect(24, 58, bitmap_width - 24, current_header_height - 14);
		DrawRouteStrip(mem_dc, route_rect, insights);
	}

	CRect summary_rect(24, current_header_height + 8, bitmap_width - 24, current_header_height + summary_height);
	CString summary_text = BuildSummary(sorted_pictures, insights);
	summary_text += L"\n";
	CString sorting_label;
	sorting_label.LoadString(IDS_SORTING_LABEL);
	CString story_line = BuildStoryLine(insights);
	summary_text += sorting_label;
	summary_text += L": ";
	summary_text += GetSortModeText(sort_mode);
	if (!story_line.IsEmpty())
	{
		summary_text += L"  |  ";
		summary_text += story_line;
	}
	mem_dc.DrawText(summary_text, summary_rect, DT_LEFT | DT_WORDBREAK);

	// Each tile reuses the requested preview plus a compact text caption and optional place badge.
	for (int index = 0; index < count; ++index)
	{
		// Rendering also stays cancelable so a large poster can still be stopped after the lookup phase.
		progress.SetStatus(LoadStringResource(IDS_PROGRESS_RENDER_PHASE), FormatStringResource(IDS_PROGRESS_RENDER_DETAIL, index + 1, count));
		progress.SetProgress(static_cast<ULONGLONG>(index + 1), static_cast<ULONGLONG>(max(1, count)));
		if (PollProgressCancellation(cancellation, progress))
			return abort_operation();

		const int col = index % cols;
		const int row = index / cols;
		const int left = tile_padding + col * (tile_width + tile_padding);
		const int top = current_header_height + summary_height + tile_padding + row * (tile_height + tile_padding);

		CRect tile_rect(left, top, left + tile_width, top + thumbnail_height);
		CRect outer_rect(left - 2, top - 2, left + tile_width + 2, top + thumbnail_height + 2);
		mem_dc.Rectangle(outer_rect);

		if (!sorted_pictures[index]->requested_data_list.empty())
			DrawRequestedPicture(mem_dc, sorted_pictures[index]->requested_data_list.front(), tile_rect);

		if (insights.entries[index].cluster_index >= 0)
		{
			const CRect badge_rect(outer_rect.left + 8, outer_rect.top + 8, outer_rect.left + 30, outer_rect.top + 30);
			DrawNumberBadge(mem_dc, badge_rect, insights.clusters[insights.entries[index].cluster_index].color, index + 1);
		}

		mem_dc.SelectObject(&caption_font);
		mem_dc.SetTextColor(RGB(52, 52, 52));
		CRect caption_rect(left, top + thumbnail_height + 8, left + tile_width, top + thumbnail_height + caption_height);
		mem_dc.DrawText(BuildCaption(*sorted_pictures[index], show_metadata, insights.entries[index], insights.clusters), caption_rect, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);
	}

	mem_dc.SelectObject(old_font);
	mem_dc.SelectObject(old_pen);
	mem_dc.SelectObject(old_bitmap);

	const CString output_name(GetDirectory(sorted_pictures.front()->file_name) + SanitizeStem(header_title) + GetExtension(sorted_pictures.front()->file_name));
	update_data_list.emplace_back(
		output_name,
		UPDATE_TYPE::UPDATE_TYPE_ADDED,
		bitmap_width,
		bitmap_height,
		reinterpret_cast<BYTE*>(dib_bits),
		DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA);
	progress.Destroy();

	return update_data_list;
}


#include "stdafx.h"
#include "Plugin.h"
#include "SettingsDlg.h"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <map>
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

	const int header_height = 138;
	const int summary_height = 66;
	const int tile_padding = 18;
	const int caption_height = 66;

	struct GeoPoint
	{
		GeoPoint() : valid(false), latitude(0.0), longitude(0.0) {}

		bool valid;
		double latitude;
		double longitude;
	};

	struct RouteCluster
	{
		RouteCluster() : latitude(0.0), longitude(0.0), count(0), color(RGB(0, 0, 0)) {}

		double latitude;
		double longitude;
		int count;
		COLORREF color;
		CString place_name;
	};

	struct PosterEntry
	{
		PosterEntry() : picture(NULL), cluster_index(-1), hop_distance_km(0.0) {}

		const picture_data* picture;
		GeoPoint geo;
		int cluster_index;
		double hop_distance_km;
	};

	struct PosterInsights
	{
		PosterInsights()
			: gps_picture_count(0), total_distance_km(0.0), longest_hop_km(0.0), longest_hop_index(-1),
			  first_exif_time(0), last_exif_time(0)
		{
		}

		vector<PosterEntry> entries;
		vector<RouteCluster> clusters;
		int gps_picture_count;
		double total_distance_km;
		double longest_hop_km;
		int longest_hop_index;
		__int64 first_exif_time;
		__int64 last_exif_time;
	};

	class ScopedWaitCursor
	{
	public:
		ScopedWaitCursor()
			: previous_cursor(::SetCursor(::LoadCursor(NULL, IDC_WAIT)))
		{
		}

		~ScopedWaitCursor()
		{
			::SetCursor(previous_cursor != NULL ? previous_cursor : ::LoadCursor(NULL, IDC_ARROW));
		}

	private:
		HCURSOR previous_cursor;
	};

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
		// Added images must carry an absolute target path; otherwise cPicture writes them into its app directory.
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
			else if (wcschr(L"\\/:*?\"<>|", ch) != NULL)
				text.SetAt(index, L'_');
		}

		return text;
	}

	COLORREF GetClusterColor(const int index)
	{
		static const COLORREF colors[] =
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

		return colors[index % (sizeof(colors) / sizeof(colors[0]))];
	}

	void ReplaceChar(CString& text, const WCHAR from, const WCHAR to)
	{
		for (int index = 0; index < text.GetLength(); ++index)
		{
			if (text[index] == from)
				text.SetAt(index, to);
		}
	}

	struct GpsNumberToken
	{
		GpsNumberToken() : value(0.0), start(0), end(0), direction(0), axis_hint(0) {}

		double value;
		int start;
		int end;
		WCHAR direction;
		int axis_hint;
	};

	bool IsNumericChar(const WCHAR ch)
	{
		return iswdigit(ch) != 0 || ch == L'+' || ch == L'-' || ch == L'.' || ch == L',';
	}

	bool IsWordLetter(const WCHAR ch)
	{
		return iswalpha(ch) != 0;
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
			if (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n' || wcschr(L":;=/()[]{}<>|,", ch) != NULL)
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
			if (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n' || wcschr(L":;=/()[]{}<>|,", ch) != NULL)
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
		ReplaceChar(text, L',', L'.');
		return text;
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

	bool TryGetNamedAddressPart(const CString& address_json, const WCHAR* key, CString& value)
	{
		if (!TryExtractJsonString(address_json, key, value))
			return false;

		value.Trim();
		return !value.IsEmpty();
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
			CString house_number;
			CString road;
			CString neighbourhood;
			CString suburb;
			CString borough;
			CString city;
			CString county;

			TryGetNamedAddressPart(address_json, L"house_number", house_number);
			if (!TryGetNamedAddressPart(address_json, L"road", road))
				if (!TryGetNamedAddressPart(address_json, L"pedestrian", road))
					if (!TryGetNamedAddressPart(address_json, L"residential", road))
						if (!TryGetNamedAddressPart(address_json, L"footway", road))
							TryGetNamedAddressPart(address_json, L"path", road);

			if (!road.IsEmpty() && !house_number.IsEmpty())
				road += L" " + house_number;

			if (!TryGetNamedAddressPart(address_json, L"neighbourhood", neighbourhood))
				if (!TryGetNamedAddressPart(address_json, L"quarter", neighbourhood))
					if (!TryGetNamedAddressPart(address_json, L"city_district", neighbourhood))
						TryGetNamedAddressPart(address_json, L"hamlet", neighbourhood);

			if (!TryGetNamedAddressPart(address_json, L"suburb", suburb))
				TryGetNamedAddressPart(address_json, L"municipality", suburb);

			TryGetNamedAddressPart(address_json, L"borough", borough);

			if (!TryGetNamedAddressPart(address_json, L"city", city))
				if (!TryGetNamedAddressPart(address_json, L"town", city))
					if (!TryGetNamedAddressPart(address_json, L"village", city))
						TryGetNamedAddressPart(address_json, L"state", city);

			TryGetNamedAddressPart(address_json, L"county", county);

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

	bool TryFetchNominatimLocationName(const GeoPoint& geo, CString& place_name)
	{
		place_name.Empty();

		HINTERNET session = ::WinHttpOpen(
			L"TimeCapsule/1.0",
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0);
		if (session == NULL)
			return false;

		::WinHttpSetTimeouts(session, 1500, 1500, 2500, 2500);

		HINTERNET connection = ::WinHttpConnect(session, L"nominatim.openstreetmap.org", INTERNET_DEFAULT_HTTPS_PORT, 0);
		if (connection == NULL)
		{
			::WinHttpCloseHandle(session);
			return false;
		}

		CString request_path;
		request_path.Format(
			L"/reverse?format=jsonv2&lat=%s&lon=%s&zoom=18&addressdetails=1&accept-language=de,en",
			FormatCoordinateForUrl(geo.latitude).GetString(),
			FormatCoordinateForUrl(geo.longitude).GetString());

		HINTERNET request = ::WinHttpOpenRequest(
			connection,
			L"GET",
			request_path,
			NULL,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			WINHTTP_FLAG_SECURE);
		if (request == NULL)
		{
			::WinHttpCloseHandle(connection);
			::WinHttpCloseHandle(session);
			return false;
		}

		const WCHAR* headers = L"Accept: application/json\r\n";
		bool ok = ::WinHttpSendRequest(request, headers, static_cast<DWORD>(-1L), NULL, 0, 0, 0) == TRUE;
		if (ok)
			ok = ::WinHttpReceiveResponse(request, NULL) == TRUE;

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

		::WinHttpCloseHandle(request);
		::WinHttpCloseHandle(connection);
		::WinHttpCloseHandle(session);

		if (!ok || status_code != 200 || response_utf8.empty())
			return false;

		place_name = PickLocationNameFromNominatimJson(Utf8ToCString(response_utf8));
		place_name.Trim();
		return !place_name.IsEmpty();
	}

	bool TryResolveLocationNameNominatim(const CString& gps_text, const GeoPoint& geo, CString& place_name)
	{
		if (TryFetchNominatimLocationName(geo, place_name))
			return true;

		if (HasExplicitGpsDirections(gps_text))
			return false;

		GeoPoint swapped_geo;
		swapped_geo.valid = true;
		swapped_geo.latitude = geo.longitude;
		swapped_geo.longitude = geo.latitude;

		if (fabs(swapped_geo.latitude) > 90.0 || fabs(swapped_geo.longitude) > 180.0)
			return false;

		return TryFetchNominatimLocationName(swapped_geo, place_name);
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
				ReplaceChar(current, L',', L'.');
				GpsNumberToken token;
				token.value = _wtof(current);
				token.start = token_start;
				token.end = index;
				token.direction = FindAdjacentDirection(input, token.start, token.end);
				token.axis_hint = FindAxisHint(input, token.start, token.end);
				values.push_back(token);
				current.Empty();
				token_start = -1;
			}
		}

		if (!current.IsEmpty())
		{
			ReplaceChar(current, L',', L'.');
			GpsNumberToken token;
			token.value = _wtof(current);
			token.start = token_start;
			token.end = input.GetLength();
			token.direction = FindAdjacentDirection(input, token.start, token.end);
			token.axis_hint = FindAxisHint(input, token.start, token.end);
			values.push_back(token);
		}

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
		CString text;
		if (km < 10.0)
			text.Format(L"%.1f km", km);
		else
			text.Format(L"%.0f km", km);
		return text;
	}

	CString FormatDuration(__int64 seconds)
	{
		if (seconds <= 0)
			return L"0 h";

		const __int64 days = seconds / 86400;
		const __int64 hours = (seconds % 86400) / 3600;
		CString text;
		if (days > 0)
			text.Format(L"%lld Tage %lld h", days, hours);
		else
			text.Format(L"%lld h", max<__int64>(1, hours));
		return text;
	}

	CString FormatLocationName(const int cluster_index)
	{
		CString text;
		text.Format(L"Ort %d", cluster_index + 1);
		return text;
	}

	CString GetClusterName(const RouteCluster& cluster, const int cluster_index)
	{
		if (!cluster.place_name.IsEmpty())
			return cluster.place_name;

		return FormatLocationName(cluster_index);
	}

	CString FormatCoordinate(const double value)
	{
		CString text;
		text.Format(L"%.4f", value);
		return text;
	}

	CString FormatLocationLabel(const RouteCluster& cluster, const int cluster_index)
	{
		if (!cluster.place_name.IsEmpty())
			return cluster.place_name;

		CString text;
		text.Format(L"%s (%s, %s)", GetClusterName(cluster, cluster_index).GetString(),
			FormatCoordinate(cluster.latitude).GetString(),
			FormatCoordinate(cluster.longitude).GetString());
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

	CString LoadText(const UINT id)
	{
		CString text;
		text.LoadString(id);
		return text;
	}

	CString GetSortModeText(const int sort_mode)
	{
		switch (sort_mode)
		{
		case SORT_MODE_SELECTION:
			return LoadText(IDS_SORT_MODE_SELECTION);
		case SORT_MODE_GPS_WEST_EAST:
			return LoadText(IDS_SORT_MODE_WEST_EAST);
		case SORT_MODE_GPS_NORTH_SOUTH:
			return LoadText(IDS_SORT_MODE_NORTH_SOUTH);
		case SORT_MODE_GPS_EAST_WEST:
			return LoadText(IDS_SORT_MODE_EAST_WEST);
		case SORT_MODE_GPS_SOUTH_NORTH:
			return LoadText(IDS_SORT_MODE_SOUTH_NORTH);
		case SORT_MODE_EXIF:
		default:
			return LoadText(IDS_SORT_MODE_EXIF);
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
		clusters.push_back(cluster);
		return static_cast<int>(clusters.size() - 1);
	}

	// Build the travel summary once so the drawing code can stay focused on layout.
	PosterInsights AnalyzePictures(const vector<const picture_data*>& pictures, const double cluster_radius_km)
	{
		PosterInsights insights;
		insights.entries.reserve(pictures.size());

		bool have_prev_geo = false;
		GeoPoint prev_geo;

		for (vector<const picture_data*>::const_iterator it = pictures.begin(); it != pictures.end(); ++it)
		{
			PosterEntry entry;
			entry.picture = *it;
			entry.geo.valid = false;

			if (TryParseDecimalGps((*it)->gps, entry.geo))
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

			if ((*it)->exif_time != 0)
			{
				if (insights.first_exif_time == 0 || (*it)->exif_time < insights.first_exif_time)
					insights.first_exif_time = (*it)->exif_time;
				if (insights.last_exif_time == 0 || (*it)->exif_time > insights.last_exif_time)
					insights.last_exif_time = (*it)->exif_time;
			}

			insights.entries.push_back(entry);
		}

		return insights;
	}

	// Reverse geocoding is done once per cluster so multi-image posters do not spam web requests.
	void ResolveClusterNames(vector<RouteCluster>& clusters, const vector<PosterEntry>& entries)
	{
		const size_t max_online_lookups = 8;
		for (size_t index = 0; index < clusters.size() && index < max_online_lookups; ++index)
		{
			CString gps_text;
			for (size_t entry_index = 0; entry_index < entries.size(); ++entry_index)
			{
				if (entries[entry_index].cluster_index == static_cast<int>(index) && entries[entry_index].picture != NULL)
				{
					gps_text = entries[entry_index].picture->gps;
					break;
				}
			}

			GeoPoint center;
			center.valid = true;
			center.latitude = clusters[index].latitude;
			center.longitude = clusters[index].longitude;

			CString place_name;
			if (TryResolveLocationNameNominatim(gps_text, center, place_name))
				clusters[index].place_name = place_name;
		}
	}

	CString BuildSummary(const vector<const picture_data*>& pictures, const PosterInsights& insights)
	{
		CString summary;
		summary.Format(L"%d Bilder", static_cast<int>(pictures.size()));

		std::map<CString, int> camera_count;

		for (vector<const picture_data*>::const_iterator it = pictures.begin(); it != pictures.end(); ++it)
		{
			CString model((*it)->model);
			model.Trim();
			if (!model.IsEmpty())
				camera_count[model]++;
		}

		CString extras;
		extras.Format(L"  |  GPS: %d  |  Orte: %d  |  Kameras: %d", insights.gps_picture_count,
			static_cast<int>(insights.clusters.size()), static_cast<int>(camera_count.size()));
		summary += extras;

		if (insights.total_distance_km > 0.0)
		{
			summary += L"  |  Strecke: ";
			summary += FormatDistance(insights.total_distance_km);
		}

		if (insights.first_exif_time != 0 && insights.last_exif_time >= insights.first_exif_time)
		{
			summary += L"  |  Dauer: ";
			summary += FormatDuration(insights.last_exif_time - insights.first_exif_time);
		}

		return summary;
	}

		CString BuildStoryLine(const PosterInsights& insights)
		{
			CString story;
			if (!insights.clusters.empty())
			{
				story = L"Start: ";
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
						story += L"  |  Ziel: ";
						story += FormatLocationLabel(insights.clusters[insights.entries[index - 1].cluster_index], insights.entries[index - 1].cluster_index);
						break;
					}
				}
			}

			if (insights.longest_hop_km > 0.0)
			{
				if (!story.IsEmpty())
					story += L"  |  ";
				story += L"Laengster Sprung: ";
				story += FormatDistance(insights.longest_hop_km);
			}

			if (story.IsEmpty())
				story = L"Keine auswertbare Route gefunden";

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

	// The route strip is intentionally schematic: a calm horizontal travel line reads better than a literal map trace.
	void DrawRouteStrip(CDC& mem_dc, const CRect& route_rect, const PosterInsights& insights)
	{
		CBrush route_background(RGB(49, 63, 78));
		mem_dc.FillRect(route_rect, &route_background);

		if (insights.gps_picture_count < 2)
		{
			mem_dc.SetBkMode(TRANSPARENT);
			mem_dc.SetTextColor(RGB(214, 219, 224));
			CRect text_rect(route_rect);
			text_rect.DeflateRect(12, 10);
			mem_dc.DrawText(L"GPS-Route erscheint hier, sobald mehrere verwertbare Punkte vorliegen.", text_rect,
				DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			return;
		}

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
			route_points.push_back(CPoint(x, baseline_y));
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
		if (request.data == NULL)
			return;

		HDRAWDIB draw_dib = DrawDibOpen();
		if (draw_dib == NULL)
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
	: handle_wnd(NULL),
	  poster_dib(NULL),
	  sort_mode(SORT_MODE_EXIF),
	  show_metadata(true),
	  thumbnail_width(240),
	  thumbnail_height(150),
	  location_cluster_radius_km(2.0)
{
}

CFunctionPluginTimeCapsule::~CFunctionPluginTimeCapsule()
{
	if (poster_dib != NULL)
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

	request_data_sizes.push_back(request_data_size(
		thumbnail_width,
		thumbnail_height,
		DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA));

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

	if (picture_data_list.size() < 2)
		return update_data_list;

	if (poster_dib != NULL)
	{
		::DeleteObject(poster_dib);
		poster_dib = NULL;
	}

	// Work on pointers so we can reorder the selected pictures without copying the plugin-owned structs.
	vector<const picture_data*> sorted_pictures;
	sorted_pictures.reserve(picture_data_list.size());
	for (vector<picture_data>::const_iterator it = picture_data_list.begin(); it != picture_data_list.end(); ++it)
		sorted_pictures.push_back(&(*it));

	SortPictures(sorted_pictures, sort_mode);

	PosterInsights insights = AnalyzePictures(sorted_pictures, location_cluster_radius_km);
	ResolveClusterNames(insights.clusters, insights.entries);

	const int count = static_cast<int>(sorted_pictures.size());
	const int cols = min(4, max(2, count));
	const int rows = (count + cols - 1) / cols;
	const int tile_width = thumbnail_width;
	const int tile_height = thumbnail_height + caption_height;
	const int bitmap_width = tile_padding + cols * (tile_width + tile_padding);
	const int bitmap_height = header_height + summary_height + tile_padding + rows * (tile_height + tile_padding);

	BITMAPINFO bitmap_info = { 0 };
	bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 24;
	bitmap_info.bmiHeader.biCompression = BI_RGB;
	bitmap_info.bmiHeader.biWidth = bitmap_width;
	bitmap_info.bmiHeader.biHeight = bitmap_height;

	void* dib_bits = NULL;
	poster_dib = ::CreateDIBSection(NULL, &bitmap_info, DIB_RGB_COLORS, &dib_bits, NULL, 0);
	if (poster_dib == NULL || dib_bits == NULL)
		return update_data_list;

	CBitmap* bitmap = CBitmap::FromHandle(poster_dib);
	CDC mem_dc;
	mem_dc.CreateCompatibleDC(NULL);
	CBitmap* old_bitmap = mem_dc.SelectObject(bitmap);

	CRect full_rect(0, 0, bitmap_width, bitmap_height);
	CBrush bg_brush(RGB(248, 245, 239));
	mem_dc.FillRect(full_rect, &bg_brush);

	CRect header_rect(0, 0, bitmap_width, header_height);
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
	CString header_title = title_text.IsEmpty() ? CString(L"Time Capsule") : title_text;
	mem_dc.DrawText(header_title, title_rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

	mem_dc.SelectObject(&summary_font);
	mem_dc.SetTextColor(RGB(49, 55, 61));
	CRect route_rect(24, 58, bitmap_width - 24, header_height - 14);
	DrawRouteStrip(mem_dc, route_rect, insights);

	CRect summary_rect(24, header_height + 8, bitmap_width - 24, header_height + summary_height);
	CString summary_text = BuildSummary(sorted_pictures, insights);
	summary_text += L"\n";
	summary_text += LoadText(IDS_SORTING_LABEL);
	summary_text += L": ";
	summary_text += GetSortModeText(sort_mode);
	summary_text += L"  |  ";
	summary_text += BuildStoryLine(insights);
	mem_dc.DrawText(summary_text, summary_rect, DT_LEFT | DT_WORDBREAK);

	// Each tile reuses the requested preview plus a compact text caption and optional place badge.
	for (int index = 0; index < count; ++index)
	{
		const int col = index % cols;
		const int row = index / cols;
		const int left = tile_padding + col * (tile_width + tile_padding);
		const int top = header_height + summary_height + tile_padding + row * (tile_height + tile_padding);

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

	// The poster is written into the folder of the first selected picture, matching the behavior of other multi-image outputs.
	CString output_name = GetDirectory(sorted_pictures.front()->file_name) + SanitizeStem(header_title) + GetExtension(sorted_pictures.front()->file_name);
	update_data_list.push_back(update_data(
		output_name,
		UPDATE_TYPE::UPDATE_TYPE_ADDED,
		bitmap_width,
		bitmap_height,
		(BYTE*)dib_bits,
		DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA));

	return update_data_list;
}


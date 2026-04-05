#include "stdafx.h"
#include "Plugin.h"
#include "SettingsDlg.h"

#include <algorithm>
#include <cmath>
#include <map>


namespace
{
	enum SORT_MODE
	{
		SORT_MODE_EXIF = 0,
		SORT_MODE_SELECTION,
		SORT_MODE_GPS_WEST_EAST,
		SORT_MODE_GPS_NORTH_SOUTH,
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
		const int pos = file_name.ReverseFind(L'\\');
		return pos >= 0 ? file_name.Mid(pos + 1) : file_name;
	}

	CString GetStem(const CString& file_name)
	{
		CString name(GetFileNameOnly(file_name));
		const int dot = name.ReverseFind(L'.');
		return dot > 0 ? name.Left(dot) : name;
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

	// GPS strings come back in different textual variants, so we first extract the numeric parts.
	bool ExtractNumbers(const CString& input, vector<double>& values)
	{
		CString current;
		for (int index = 0; index < input.GetLength(); ++index)
		{
			const WCHAR ch = input[index];
			if ((ch >= L'0' && ch <= L'9') || ch == L'+' || ch == L'-' || ch == L'.' || ch == L',')
			{
				current += ch;
			}
			else if (!current.IsEmpty())
			{
				ReplaceChar(current, L',', L'.');
				values.push_back(_wtof(current));
				current.Empty();
			}
		}

		if (!current.IsEmpty())
		{
			ReplaceChar(current, L',', L'.');
			values.push_back(_wtof(current));
		}

		return values.size() >= 2;
	}

	// The plugin only needs a tolerant decimal lat/lon parser, not full GPS DMS support.
	bool TryParseDecimalGps(const CString& gps_text, GeoPoint& point)
	{
		vector<double> values;
		if (!ExtractNumbers(gps_text, values))
			return false;

		double latitude = values[0];
		double longitude = values[1];

		CString upper(gps_text);
		upper.MakeUpper();
		if (upper.Find(L'S') >= 0)
			latitude = -fabs(latitude);
		if (upper.Find(L'W') >= 0)
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

	CString FormatCoordinate(const double value)
	{
		CString text;
		text.Format(L"%.4f", value);
		return text;
	}

	CString FormatLocationLabel(const RouteCluster& cluster, const int cluster_index)
	{
		CString text;
		text.Format(L"Ort %d (%s, %s)", cluster_index + 1,
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

	CString BuildCaption(const picture_data& picture, bool show_metadata, const PosterEntry& entry)
	{
		CString line1(GetStem(picture.file_name));
		CString line2;
		CString line3;

		if (!picture.exif_datetime_display.IsEmpty())
			line2 = picture.exif_datetime_display;
		else
			line2.Format(L"%d x %d", picture.picture_width, picture.picture_height);

		if (show_metadata)
		{
			CString location;
			if (entry.cluster_index >= 0)
				location = FormatLocationName(entry.cluster_index);

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

	bool ComparePicturesByGps(const picture_data* lhs, const picture_data* rhs, const bool west_east)
	{
		GeoPoint lhs_geo;
		GeoPoint rhs_geo;
		const bool lhs_has_geo = TryParseDecimalGps(lhs->gps, lhs_geo);
		const bool rhs_has_geo = TryParseDecimalGps(rhs->gps, rhs_geo);

		if (lhs_has_geo != rhs_has_geo)
			return lhs_has_geo > rhs_has_geo;

		if (lhs_has_geo && rhs_has_geo)
		{
			const double lhs_axis = west_east ? lhs_geo.longitude : lhs_geo.latitude;
			const double rhs_axis = west_east ? rhs_geo.longitude : rhs_geo.latitude;
			if (lhs_axis != rhs_axis)
				return lhs_axis < rhs_axis;

			const double lhs_secondary = west_east ? lhs_geo.latitude : lhs_geo.longitude;
			const double rhs_secondary = west_east ? rhs_geo.latitude : rhs_geo.longitude;
			if (lhs_secondary != rhs_secondary)
				return lhs_secondary < rhs_secondary;
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
				[](const picture_data* lhs, const picture_data* rhs) { return ComparePicturesByGps(lhs, rhs, true); });
			break;
		case SORT_MODE_GPS_NORTH_SOUTH:
			std::stable_sort(pictures.begin(), pictures.end(),
				[](const picture_data* lhs, const picture_data* rhs) { return ComparePicturesByGps(lhs, rhs, false); });
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

	// Route positions are normalized into the banner rect instead of using a real map projection.
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

		double min_lat = 999.0;
		double max_lat = -999.0;
		double min_lon = 999.0;
		double max_lon = -999.0;

		for (size_t index = 0; index < insights.entries.size(); ++index)
		{
			if (!insights.entries[index].geo.valid)
				continue;
			min_lat = min(min_lat, insights.entries[index].geo.latitude);
			max_lat = max(max_lat, insights.entries[index].geo.latitude);
			min_lon = min(min_lon, insights.entries[index].geo.longitude);
			max_lon = max(max_lon, insights.entries[index].geo.longitude);
		}

		const double lat_span = max(0.001, max_lat - min_lat);
		const double lon_span = max(0.001, max_lon - min_lon);
		const int left = route_rect.left + 14;
		const int top = route_rect.top + 14;
		const int width = max(1, route_rect.Width() - 28);
		const int height = max(1, route_rect.Height() - 28);

		CPen route_pen(PS_SOLID, 3, RGB(225, 214, 170));
		CPen* old_pen = mem_dc.SelectObject(&route_pen);
		bool first = true;
		CPoint prev;

		for (size_t index = 0; index < insights.entries.size(); ++index)
		{
			if (!insights.entries[index].geo.valid)
				continue;

			const int x = left + static_cast<int>(((insights.entries[index].geo.longitude - min_lon) / lon_span) * width);
			const int y = top + height - static_cast<int>(((insights.entries[index].geo.latitude - min_lat) / lat_span) * height);

			if (first)
			{
				mem_dc.MoveTo(x, y);
				first = false;
			}
			else
			{
				mem_dc.LineTo(x, y);
			}
			prev = CPoint(x, y);
		}

		mem_dc.SelectObject(old_pen);

		for (size_t index = 0; index < insights.entries.size(); ++index)
		{
			if (!insights.entries[index].geo.valid || insights.entries[index].cluster_index < 0)
				continue;

			const int x = left + static_cast<int>(((insights.entries[index].geo.longitude - min_lon) / lon_span) * width);
			const int y = top + height - static_cast<int>(((insights.entries[index].geo.latitude - min_lat) / lat_span) * height);
			const int badge_radius = 10;
			const CRect badge_rect(x - badge_radius, y - badge_radius, x + badge_radius + 1, y + badge_radius + 1);
			DrawNumberBadge(mem_dc, badge_rect, insights.clusters[insights.entries[index].cluster_index].color, static_cast<int>(index) + 1);
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

	const PosterInsights insights = AnalyzePictures(sorted_pictures, location_cluster_radius_km);

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
		mem_dc.DrawText(BuildCaption(*sorted_pictures[index], show_metadata, insights.entries[index]), caption_rect, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);
	}

	mem_dc.SelectObject(old_font);
	mem_dc.SelectObject(old_pen);
	mem_dc.SelectObject(old_bitmap);

	CString output_name = SanitizeStem(header_title) + GetExtension(sorted_pictures.front()->file_name);
	update_data_list.push_back(update_data(
		output_name,
		UPDATE_TYPE::UPDATE_TYPE_ADDED,
		bitmap_width,
		bitmap_height,
		(BYTE*)dib_bits,
		DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA));

	return update_data_list;
}


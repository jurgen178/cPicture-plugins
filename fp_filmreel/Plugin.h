#pragma once
#include "FunctionPlugin.h"


struct AnimSettings
{
	int  delay_ms     = 200;   // frame delay in milliseconds
	int  loop_count   = 0;     // 0 = infinite
	int  quality      = 80;    // 0-100, ignored when lossless = true
	bool lossless     = false;
	int  output_width = 1920;  // max output width in pixels; 0 = original full resolution
	COLORREF background_color = RGB(0, 0, 0); // used for canvas areas around differently sized frames
};


class CFunctionPluginFilmReel : public CFunctionPlugin
{
protected:
	CFunctionPluginFilmReel();

protected:
	HWND        handle_wnd;
	AnimSettings m_settings;

public:
	virtual ~CFunctionPluginFilmReel() { }

	static CFunctionPlugin* __stdcall GetInstance()
	{
		return new CFunctionPluginFilmReel;
	}

public:
	virtual struct plugin_data __stdcall get_plugin_data() const;
	virtual struct arg_count   __stdcall get_arg_count()   const;

private:
	void LoadSettings();
	void SaveSettings() const;

public:
	virtual enum REQUEST_TYPE          __stdcall start(const HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes);
	virtual bool                       __stdcall process_picture(const picture_data& picture_data);
	virtual const vector<update_data>& __stdcall end(const vector<picture_data>& picture_data_list);
};

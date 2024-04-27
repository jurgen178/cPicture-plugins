#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include "ParameterDlg.h"

// Plugin cpp_fp_hdr

// https://sourceforge.net/projects/enblend/
// 
//Usage: enfuse [options] [--output=IMAGE] INPUT...
//Fuse INPUT images into a single IMAGE.
//
//INPUT... are image filenames or response filenames.  Response
//filenames start with an "@" character.
//
//Options:
//Common options:
//  -l, --levels=LEVELS    limit number of blending LEVELS to use (1 to 29);
//                         negative number of LEVELS decreases maximum;
//                         "auto" restores the default automatic maximization
//  -o, --output=FILE      write output to FILE; default: "a.tif"
//  -v, --verbose[=LEVEL]  verbosely report progress; repeat to
//                         increase verbosity or directly set to LEVEL
//  --compression=COMPRESSION
//                         set compression of output image to COMPRESSION,
//                         where COMPRESSION is:
//                         "deflate", "jpeg", "lzw", "none", "packbits", for TIFF files and
//                         0 to 100, or "jpeg", "jpeg-arith" for JPEG files,
//                         where "jpeg" and "jpeg-arith" accept a compression level
//
//Advanced options:
//  --blend-colorspace=COLORSPACE
//                         force COLORSPACE for blending operations; Enfuse uses
//                         "CIELUV" for images with ICC-profile and "IDENTITY" for
//                         those without and also for all floating-point images;
//                         other available blend color spaces are "CIELAB" and
//                         "CIECAM"
//  -c, --ciecam           use CIECAM02 to blend colors; disable with "--no-ciecam";
//                         note that this option will be withdrawn in favor of
//                         "--blend-colorspace"
//  -d, --depth=DEPTH      set the number of bits per channel of the output
//                         image, where DEPTH is "8", "16", "32", "r32", or "r64"
//  -f WIDTHxHEIGHT[+xXOFFSET+yYOFFSET]
//                         manually set the size and position of the output
//                         image; useful for cropped and shifted input
//                         TIFF images, such as those produced by Nona
//  -g                     associated-alpha hack for Gimp (before version 2)
//                         and Cinepaint
//  -w, --wrap[=MODE]      wrap around image boundary, where MODE is "none",
//                         "horizontal", "vertical", or "both"; default: none;
//                         without argument the option selects horizontal wrapping
//
//Fusion options:
//  --exposure-weight=WEIGHT
//                         weight given to well-exposed pixels
//                         (0 <= WEIGHT <= 1); default: 1
//  --saturation-weight=WEIGHT
//                         weight given to highly-saturated pixels
//                         (0 <= WEIGHT <= 1); default: 0.2
//  --contrast-weight=WEIGHT
//                         weight given to pixels in high-contrast neighborhoods
//                         (0 <= WEIGHT <= 1); default: 0
//  --entropy-weight=WEIGHT
//                         weight given to pixels in high entropy neighborhoods
//                         (0 <= WEIGHT <= 1); default: 0
//  --exposure-optimum=OPTIMUM
//                         optimum exposure value, usually the maximum of the weighting
//                         function (0 <= OPTIMUM <= 1); default: 0.5
//  --exposure-width=WIDTH
//                         characteristic width of the weighting function
//                         (WIDTH > 0); default: 0.2
//  --soft-mask            average over all masks; this is the default
//  --hard-mask            force hard blend masks and no averaging on finest
//                         scale; this is especially useful for focus
//                         stacks with thin and high contrast features,
//                         but leads to increased noise
//
//Expert options:
//  --save-masks[=SOFT-TEMPLATE[:HARD-TEMPLATE]]
//                         save weight masks in SOFT-TEMPLATE and HARD-TEMPLATE;
//                         conversion chars: "%i": mask index, "%n": mask number,
//                         "%p": full path, "%d": dirname, "%b": basename,
//                         "%f": filename, "%e": extension; lowercase characters
//                         refer to input images uppercase to the output image
//                         default: "softmask-%n.tif":"hardmask-%n.tif"
//  --load-masks[=SOFT-TEMPLATE[:HARD-TEMPLATE]]
//                         skip calculation of weight maps and use the ones
//                         in the files matching the templates instead.  These
//                         can be either hard or soft masks.  For template
//                         syntax see "--save-masks";
//                         default: "softmask-%n.tif":"hardmask-%n.tif"
//  --fallback-profile=PROFILE-FILE
//                         use the ICC profile from PROFILE-FILE instead of sRGB
//  --layer-selector=ALGORITHM
//                         set the layer selector ALGORITHM;
//                         default: "all-layers"; available algorithms are:
//                         "all-layers": select all layers in any image;
//                         "first-layer": select only first layer in each (multi-)layer image;
//                         "last-layer": select only last layer in each (multi-)layer image;
//                         "largest-layer": select largest layer in each (multi-)layer image;
//                         "no-layer": do not select any layer from any image;
//  --parameter=KEY1[=VALUE1][:KEY2[=VALUE2][:...]]
//                         set one or more KEY-VALUE pairs
//
//Expert fusion options:
//  --exposure-weight-function=WEIGHT-FUNCTION    (1st form)
//                         select one of the built-in exposure WEIGHT-FUNCTIONs:
//                         "gaussian", "lorentzian", "half-sine", "full-sine",
//                         or "bi-square"; default: "gaussian"
//  --exposure-weight-function=SHARED-OBJECT:SYMBOL[:ARGUMENT[:...]]    (2nd form)
//                         load user-defined exposure weight function SYMBOL
//                         from SHARED-OBJECT and optionally pass ARGUMENTs
//  --exposure-cutoff=LOWERCUTOFF[:UPPERCUTOFF[:LOWERPROJECTOR[:UPPERPROJECTOR]]]
//                         LOWERCUTOFF and UPPERCUTOFF are the values below
//                         or above of which pixels are weighted with zero
//                         weight in exposure weighting; append "%" signs
//                         for relative values; default: 0%:100%:anti-value:value
//  --contrast-window-size=SIZE
//                         set window SIZE for local-contrast analysis
//                         (SIZE >= 3); default: 5
//  --contrast-edge-scale=EDGESCALE[:LCESCALE[:LCEFACTOR]]
//                         set scale on which to look for edges; positive
//                         LCESCALE switches on local contrast enhancement
//                         by LCEFACTOR (EDGESCALE, LCESCALE, LCEFACTOR >= 0);
//                         append "%" to LCESCALE for values relative to
//                         EDGESCALE; append "%" to LCEFACTOR for relative
//                         value; default: 0:0:0
//  --contrast-min-curvature=CURVATURE
//                         minimum CURVATURE for an edge to qualify; append
//                         "%" for relative values; default: 0
//  --gray-projector=PROJECTOR
//                         apply gray-scale PROJECTOR in exposure or contrast
//                         weighing, where PROJECTOR is one of "anti-value",
//                         "average", "l-star", "lightness", "luminance",
//                         "pl-star", "value", or
//                         "channel-mixer:RED-WEIGHT:GREEN-WEIGHT:BLUE-WEIGHT";
//                         default: "average"
//  --entropy-window-size=SIZE
//                         set window SIZE for local entropy analysis
//                         (SIZE >= 3); default: 3
//  --entropy-cutoff=LOWERCUTOFF[:UPPERCUTOFF]
//                         LOWERCUTOFF is the value below of which pixels are
//                         treated as black and UPPERCUTOFF is the value above
//                         of which pixels are treated as white in the entropy
//                         weighting; append "%" signs for relative values;
//                         default: 0%:100%
//
//Information options:
//  -h, --help             print this help message and exit
//  -V, --version          output version information and exit
//  --show-globbing-algorithms
//                         show all globbing algorithms
//  --show-image-formats   show all recognized image formats and their filename
//                         extensions
//  --show-signature       show who compiled the binary when and on which machine
//  --show-software-components
//                         show the software components with which Enfuse was compiled
//
//Enfuse accepts arguments to any option in uppercase as
//well as in lowercase letters.
//
//Environment:
//  OMP_NUM_THREADS        The OMP_NUM_THREADS environment variable sets the number
//                         of threads to use in OpenMP parallel regions.  If unset
//                         Enfuse uses as many threads as there are CPUs.
//  OMP_DYNAMIC            The OMP_DYNAMIC environment variable controls dynamic
//                         adjustment of the number of threads to use in executing
//                         OpenMP parallel regions.
//
//Report bugs at <https://bugs.launchpad.net/enblend>.//


CWinApp theApp;	// für DoModal, sonst wird NULL-pointer dereferenziert.


const CString __stdcall GetPluginVersion()
{
	return L"1.0";
}

const CString __stdcall GetPluginInterfaceVersion()
{
	return L"1.6";
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
	return CFunctionPluginHDR::GetInstance;
}


CFunctionPluginHDR::CFunctionPluginHDR()
	: m_hwnd(NULL)
{
	_wsetlocale(LC_ALL, L".ACP");
}

struct PluginData __stdcall CFunctionPluginHDR::get_plugin_data()
{
	struct PluginData pluginData;

	// Set plugin info.
	pluginData.name.LoadString(IDS_PLUGIN_SHORT_DESC);
	pluginData.desc.LoadString(IDS_PLUGIN_LONG_DESC);
	pluginData.info.LoadString(IDS_PLUGIN_INFO);

	return pluginData;
}

struct request_info __stdcall CFunctionPluginHDR::start(HWND hwnd, const vector<const WCHAR*>& file_list)
{
	m_hwnd = hwnd;

	// Requires at least 2 pictures.
	if (file_list.size() < 2)
	{
		AfxMessageBox(IDS_MIN_SELECTION, MB_ICONINFORMATION);
		return request_info(PICTURE_LAYOUT_CANCEL_REQUEST);
	}

	picture_list = file_list;

	return request_info();
}

bool __stdcall CFunctionPluginHDR::process_picture(const picture_data& _picture_data)
{
	// Return true to continue, return false to stop with this picture and continue to the 'end' event.
	return true;
}

const vector<update_info>& __stdcall CFunctionPluginHDR::end()
{
	// Run enfuse with the selected pictures.
	if (picture_list.size() >= 2)
	{
		CWnd parent;
		parent.Attach(m_hwnd);

		ParameterDlg parameterDlg(picture_list, &parent);

		const CString bild_1(picture_list[0]);
		const CString bild_n(picture_list[picture_list.size() - 1]);
		const CString dir_bild(bild_1.Left(bild_1.ReverseFind(L'\\') + 1));

		CString name_bild_1(bild_1.Mid(bild_1.ReverseFind(L'\\') + 1));
		name_bild_1 = name_bild_1.Left(name_bild_1.ReverseFind(L'.'));

		CString name_bild_n(bild_n.Mid(bild_n.ReverseFind(L'\\') + 1));
		name_bild_n = name_bild_n.Left(name_bild_n.ReverseFind(L'.'));

		const CString ext_bild_1(bild_1.Mid(bild_1.ReverseFind(L'.')));
		const CString ldr_file(dir_bild + name_bild_1 + L"-" + name_bild_n + ext_bild_1);

		parameterDlg.m_OutputFile = ldr_file;
		parameterDlg.m_JpegQuality = 95;

		const CString path(L".");
		WCHAR abs_path[MAX_PATH] = { 0 };
		if (_wfullpath(abs_path, path, MAX_PATH - 1) == NULL)
			wcsncpy_s(abs_path, MAX_PATH, path, MAX_PATH - 1);

		parameterDlg.m_Enfuse = CString(abs_path) + L"\\enfuse.exe";

		if (parameterDlg.DoModal() == IDOK)
		{
			m_update_info.push_back(update_info(parameterDlg.m_OutputFile, UPDATE_TYPE_ADDED));
		}

		parent.Detach();
	}

	return m_update_info;
}

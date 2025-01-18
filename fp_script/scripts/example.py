"""
Description: Example python script to print the picture data
"""

# plugin variables
# Do not remove the leading # of the variable #[...]:

# console=true (default) displays a console, use this option for scripts with text output.
#[console=true]

# noexit=true keeps the console open, set to 'false' (default) to have the console closed when processing is done. This variable is only used when #[console=true].
#[noexit=true]

import json
import sys
import base64

# Get the picture data.
def picture_data(base64_string):

    # Decode base64 string
    json_string = base64.b64decode(base64_string).decode('utf-8').rstrip('\0')

    # Parse JSON string
    data = json.loads(json_string)

    # Print the number of pictures.
    print(f"{len(data)} picture(s): {"-" * 15}")
    print()

    # Print the picture data.
    for i, item in enumerate(data):

        # Example:
        # item["file"]   : c:\Bilder\bild1.jpg
        # item["name"]   : bild1.jpg
        # item["dir"]    : c:\Bilder\
        # item["width"]  : 3712
        # item["height"] : 5568

        # item["audio"]        : false
        # item["video"]        : false
        # item["colorprofile"] : false
        # item["gps"]          : N 47° 37' 0,872498" W 122° 19' 32,021484"
        # item["aperture"]     : 5.6   # f/5.6
        # item["shutterspeed"] : 1250  # 1/1250s
        # item["iso"]          : 100
        # item["exifdate"]     : 133553225690000000
        # item["exifdate_str"] : 19.03.2024 11:49:29
        # item["model"]        : NIKON Z 30 
        # item["lens"]         : 16-50mm f/3,5-6,3 VR f=44mm/66mm
        # item["cdata"]        :  # Configurable with F9
            
        mp = int(item["width"] * item["height"] / 1000000)
        print(f"  Image '{item["file"]}' ({i+1} of {len(data)}) with {item["width"]}x{item["height"]} pixel ({mp}MP)")   
        print(f"    name='{item["name"]}', dir='{item["dir"]}'")
        print()

        print(f"Picture {i}:")
        for key, value in item.items():
            print(f"  {key}: {value}")
        print()

        # Use specific cdata field.
        element_name = "Modell"
        if 'cdata' in item and len(item['cdata']) > 0:
            element_value = item['cdata'][0].get(element_name, None)
            print(f"Value of cdata element '{element_name}': {element_value}")
        else:
            print(f"cdata element '{element_name}' not found.")

        # Access and print cdata elements if they exist.
        if 'cdata' in item:
            print(f"{len(item['cdata'][0])} elements in cdata")
            for i, cdata_item in enumerate(item['cdata']):
                for key, value in cdata_item.items():
                    print(f"  {key}: {value}")     

        print("-" * 70)


if __name__ == "__main__":
    if len(sys.argv) == 2:
        picture_data(sys.argv[1])

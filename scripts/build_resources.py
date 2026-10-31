import os
from pathlib import Path

IMAGE_NAMES = [
    ("APPICON", "icon.png"),
    ("FILE", "file_poly.png"),
    ("FOLDER", "folder-open_poly.png"),
    ("SAVE", "save_poly.png"),
]
WIN32_INDEX_START = 100
GTK_PREFIX = "/images"
HEADER_PREFIX = "WTK_RSRC_"

WIN32_RC_PATH = "rsrc/shex.rc"
GTK_RC_PATH = "rsrc/shex_gresource.xml"
ASSETS_HEADER_PATH = "src/wtk/wtk_assets.h"

project_dir = Path(__file__).resolve().parent.parent


def build_win32_resources():
    rsrc = '1 24 "shex.manifest"'
    for idx, image_name in enumerate(IMAGE_NAMES):
        ico_path = str(Path(image_name[1]).with_suffix(".ico"))
        rsrc += f'\n{idx + WIN32_INDEX_START} ICON "{ico_path}"'

    rsrc += """
VS_VERSION_INFO VERSIONINFO
    FILEVERSION 1,0,0,0
    PRODUCTVERSION 1,0,0,0
    FILEOS 0x40004
    FILETYPE 0x1
{
    BLOCK "StringFileInfo"
    {
        BLOCK "040904b0"
        {
            VALUE "CompanyName", "Fried Chickens"
            VALUE "FileDescription", "SpookyHex"
            VALUE "FileVersion", "1.0.0.0"
            VALUE "OriginalFilename", "shex.exe"
            VALUE "ProductName", "SpookyHex"
            VALUE "ProductVersion", "1.0.0.0"
        }
    }
    BLOCK "VarFileInfo"
    {
        VALUE "Translation", 0x409, 1200
    }
}
"""

    inc = "#if wtk_win32"
    for idx, image_name in enumerate(IMAGE_NAMES):
        inc += f"\n#define {HEADER_PREFIX}{image_name[0]} {idx + WIN32_INDEX_START}"

    inc += "\n#endif // wtk_win32\n"

    with open(project_dir / WIN32_RC_PATH, "w") as fo:
        fo.write(rsrc)

    with open(project_dir / ASSETS_HEADER_PATH, "a") as fo:
        fo.write(inc)


def build_gtk_resources():
    xml = f"""<?xml version="1.0" encoding="UTF-8"?>
<gresources>
  <gresource prefix="{GTK_PREFIX}">"""
    for image_name in IMAGE_NAMES:
        xml += f"\n    <file>assets/{image_name[1]}</file>"

    xml += """
  </gresource>
</gresources>
"""

    inc = "#if wtk_gtk"
    for image_name in IMAGE_NAMES:
        inc += (
            f'\n#define {HEADER_PREFIX}{image_name[0]} "{GTK_PREFIX}/{image_name[1]}"'
        )

    inc += "\n#endif // wtk_gtk\n"

    with open(project_dir / GTK_RC_PATH, "w") as fo:
        fo.write(xml)

    with open(project_dir / ASSETS_HEADER_PATH, "a") as fo:
        fo.write(inc)


def build_all_resources():
    try:
        os.remove(project_dir / ASSETS_HEADER_PATH)
    except OSError:
        pass

    with open(project_dir / ASSETS_HEADER_PATH, "a") as fo:
        fo.write("#ifndef WTK_ASSETS_H\n#define WTK_ASSETS_H\n\n")

    build_win32_resources()
    build_gtk_resources()

    with open(project_dir / ASSETS_HEADER_PATH, "a") as fo:
        fo.write("\n#endif // WTK_ASSETS_H\n")


if __name__ == "__main__":
    build_all_resources()

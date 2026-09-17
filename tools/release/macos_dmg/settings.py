from pathlib import Path


def required_path(name):
    value = defines.get(name)
    if not value:
        raise RuntimeError(f"missing dmgbuild define: {name}")
    path = Path(value).resolve(strict=True)
    return str(path)


app_path = required_path("app_path")
documentation_path = required_path("documentation_path")
background_path = required_path("background_path")
volume_icon_path = required_path("volume_icon_path")

format = "UDZO"
compression_level = 9
filesystem = "HFS+"
size = None

files = [
    (app_path, "Qwertycoin.app"),
    (documentation_path, "Documentation"),
]
symlinks = {"Applications": "/Applications"}

icon = volume_icon_path
background = background_path

window_rect = ((120, 120), (700, 448))
default_view = "icon-view"
show_icon_preview = False
show_status_bar = False
show_tab_view = False
show_toolbar = False
show_pathbar = False
show_sidebar = False
sidebar_width = 180

arrange_by = None
grid_offset = (0, 0)
grid_spacing = 100
scroll_position = (0, 0)
label_pos = "bottom"
text_size = 13
icon_size = 110
icon_locations = {
    "Qwertycoin.app": (150, 210),
    "Applications": (550, 210),
    "Documentation": (350, 310),
}

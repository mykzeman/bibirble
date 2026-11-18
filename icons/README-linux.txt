Adding Bibirble to your Linux app menu (optional)

The Bibirble program already runs on its own and shows its icon in the
window and taskbar. To also get a menu entry with the icon, run these
from the folder you unpacked:

  mkdir -p ~/.local/bin ~/.local/share/applications ~/.local/share/icons/hicolor/512x512/apps
  cp Bibirble ~/.local/bin/
  cp bibirble.png ~/.local/share/icons/hicolor/512x512/apps/
  cp bibirble.desktop ~/.local/share/applications/

You may need to log out and back in before it shows up.

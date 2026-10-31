# Window icons

These icons come from the GNOME Project's Adwaita icon theme:
<https://www.gnome.org> · <https://gitlab.gnome.org/GNOME/adwaita-icon-theme>

- `m2mtools.svg`: `scalable/places/network-workgroup.svg`
- `event-log.svg`: `scalable/mimetypes/text-x-generic.svg`

The SVG artwork is unchanged. PNG and multiresolution ICO files are format
conversions for Qt window icons and the Windows executable. Attribution and
license terms are preserved in [LICENSE-ADWAITA.txt](LICENSE-ADWAITA.txt).

To regenerate with librsvg and ImageMagick:

```sh
rsvg-convert -w 256 -h 256 -o m2mtools.png m2mtools.svg
rsvg-convert -w 256 -h 256 -o event-log.png event-log.svg
magick m2mtools.png -define icon:auto-resize=256,128,64,48,32,24,16 m2mtools.ico
magick event-log.png -define icon:auto-resize=256,128,64,48,32,24,16 event-log.ico
```

# Window icons

The application icon uses the approved M2M family artwork: a navy state graph
and coral pencil on an ivory rounded square. The matching M2MConvertor icon
uses mint conversion arrows.

`m2mtools-source.png` is the generated master artwork with transparent outer
corners. `m2mtools.png` is the 256 px Qt resource.

`m2mtools-small-source.png` is the compact drawing for window title bars, with
a larger pictogram, thicker strokes and less whitespace. The 16, 20, 24 and
32 px PNG resources use this drawing. Qt selects the matching size for the
screen scale. The Windows title-bar icon dimensions are controlled by Windows.

`m2mtools.ico` combines the compact drawing at 16–32 px and the standard drawing
at 40, 48, 64, 128 and 256 px. It is embedded in the Windows executable.

The event-log icon comes from the GNOME Project's Adwaita icon theme:
<https://www.gnome.org> · <https://gitlab.gnome.org/GNOME/adwaita-icon-theme>

- `event-log.svg`: `scalable/mimetypes/text-x-generic.svg`

The event-log SVG artwork is unchanged. Its PNG and multiresolution ICO files
are format conversions. Attribution and
license terms are preserved in [LICENSE-ADWAITA.txt](LICENSE-ADWAITA.txt).

To regenerate with librsvg and ImageMagick:

```sh
magick m2mtools-source.png -resize 256x256 m2mtools.png
rsvg-convert -w 256 -h 256 -o event-log.png event-log.svg
for icon_size in 16 20 24 32; do
    magick m2mtools-small-source.png -resize "${icon_size}x${icon_size}" "m2mtools-${icon_size}.png"
done
magick m2mtools-16.png m2mtools-20.png m2mtools-24.png m2mtools-32.png \
    \( m2mtools.png -resize 40x40 \) \( m2mtools.png -resize 48x48 \) \
    \( m2mtools.png -resize 64x64 \) \( m2mtools.png -resize 128x128 \) \
    m2mtools.png m2mtools.ico
magick event-log.png -define icon:auto-resize=256,128,64,48,32,24,16 event-log.ico
```

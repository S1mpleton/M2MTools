# Window icons

The application icon uses the approved M2M family artwork: a navy state graph
and coral pencil on an ivory rounded square. The matching M2MConvertor icon
uses mint conversion arrows.

`m2mtools-source.png` is the generated master artwork with transparent outer
corners. `m2mtools.png` is the 256 px Qt resource. `m2mtools.ico` contains 16, 24,
32, 48, 64, 128 and 256 px images and is embedded in the Windows executable.

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
magick m2mtools-source.png -define icon:auto-resize=256,128,64,48,32,24,16 m2mtools.ico
magick event-log.png -define icon:auto-resize=256,128,64,48,32,24,16 event-log.ico
```

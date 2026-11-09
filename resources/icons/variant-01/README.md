# Variant 01 — active application icon

This is the first approved icon pair: a white M2M monogram across a diagonal
charcoal/silver square. M2MTools uses a blue reaction sheet and pencil;
M2MConvertor uses blue conversion arrows and state diagrams.

`m2mtools-source.png` is the master artwork. `m2mtools.png` is the 256 px Qt
resource. The 16, 20, 24 and 32 px PNG files serve native small-icon sizes;
`m2mtools.ico` also contains 40, 48, 64, 128 and 256 px frames for Windows.

The previous state-graph icon set is preserved unchanged in the parent directory.

Regenerate the exported formats with ImageMagick:

```sh
magick m2mtools-source.png -resize 256x256 m2mtools.png
for icon_size in 16 20 24 32; do
    magick m2mtools-source.png -resize "${icon_size}x${icon_size}" "m2mtools-${icon_size}.png"
done
magick m2mtools-16.png m2mtools-20.png m2mtools-24.png m2mtools-32.png \
    \( m2mtools.png -resize 40x40 \) \( m2mtools.png -resize 48x48 \) \
    \( m2mtools.png -resize 64x64 \) \( m2mtools.png -resize 128x128 \) \
    m2mtools.png m2mtools.ico
```

mkdir -p tmp
magick convert -geometry 16x16 SCAppIcon.png tmp/icon_16x16.png
magick convert -geometry 32x32 SCAppIcon.png tmp/icon_32x32.png
magick convert -geometry 64x64 SCAppIcon.png tmp/icon_64x64.png
magick convert -geometry 128x128 SCAppIcon.png tmp/icon_128x128.png
magick convert -geometry 256x256 SCAppIcon.png tmp/icon_256x256.png
magick convert -geometry 512x512 SCAppIcon.png tmp/icon_512x512.png
magick convert -geometry 1024x1024 SCAppIcon.png tmp/icon_1024x1024.png

mkdir -p SCAppIcon.iconset

# Copy the generated PNGs into the icon set directory, renaming them according to macOS conventions
cp tmp/icon_16x16.png SCAppIcon.iconset/icon_16x16.png
cp tmp/icon_32x32.png SCAppIcon.iconset/icon_16x16@2x.png # 32x32 is 16x16@2x
cp tmp/icon_32x32.png SCAppIcon.iconset/icon_32x32.png
cp tmp/icon_64x64.png SCAppIcon.iconset/icon_32x32@2x.png # 64x64 is 32x32@2x
cp tmp/icon_128x128.png SCAppIcon.iconset/icon_128x128.png
cp tmp/icon_256x256.png SCAppIcon.iconset/icon_128x128@2x.png # 256x256 is 128x128@2x
cp tmp/icon_256x256.png SCAppIcon.iconset/icon_256x256.png
cp tmp/icon_512x512.png SCAppIcon.iconset/icon_256x256@2x.png # 512x512 is 256x256@2x
cp tmp/icon_512x512.png SCAppIcon.iconset/icon_512x512.png
cp tmp/icon_1024x1024.png SCAppIcon.iconset/icon_512x512@2x.png # 1024x1024 is 512x512@2x

# Generate the .icns file
iconutil -c icns SCAppIcon.iconset
rm -rf tmp
rm -rf SCAppIcon.iconset
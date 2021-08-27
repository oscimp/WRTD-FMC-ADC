mkdir -p modules/extra/triggers
mkdir -p modules/extra/buffers
mkdir -p modules/extra/devices

find $BUILD_DIR -name *.ko | xargs cp -t modules/extra
mv modules/extra/zio-trig-* modules/extra/triggers
mv modules/extra/zio-buf-* modules/extra/buffers
mv modules/extra/zio-* modules/extra/devices

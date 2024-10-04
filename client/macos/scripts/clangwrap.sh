#!/bin/sh

# go/clangwrap_macos.sh

# Lấy đường dẫn SDK cho macOS
SDK_PATH=`xcrun --sdk macosx --show-sdk-path`

# Tìm đường dẫn đến `clang` cho macOS
CLANG=`xcrun --sdk macosx --find clang`

# Xác định kiến trúc máy dựa trên biến GOARCH
if [ "$GOARCH" == "amd64" ]; then
    CARCH="x86_64"
elif [ "$GOARCH" == "arm64" ]; then
    CARCH="arm64"
fi

# Thực thi `clang` với các tùy chọn cụ thể cho macOS
exec $CLANG -arch $CARCH -isysroot $SDK_PATH -mmacosx-version-min=10.15 "$@"

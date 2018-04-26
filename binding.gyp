{
    "targets": [{
        "target_name": "binobject",
        "cflags": ["-lm"],
        "include_dirs": ["<!(node -e \"require('nan')\")"],
        "sources": [
            "./src/encoder.c",
            "./src/decoder.c",
            "./src/ieee754.c",
            "./src/custom-type.cpp",
            "./src/node-encoder.cc",
            "./src/node-decoder.cc",
            "./src/node-binobject.cc"
        ]
    }]
}
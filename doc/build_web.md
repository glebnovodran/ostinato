# Web build

A web version of Ostinato can be built with [Emscripten](https://emscripten.org/).

## Build prerequisites

You'll need to install [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) and have environment variable EMSDK set to the SDK location

## Compilation
To compile to webassembly

`./build.sh wasm`

To compile to JavaScript

`./build.sh js`

Output files ostinato.html and ostinato.data will be placed to bin folder.

## Launching

You can try the web version localy by first launching a Python web-server:

`cd bin/`
`python3 -m http.server`

Access the web application at 
http://localhost:8000/ostinato.html

You can pass *small*, *low*, and *vl* parameters to the page one by one or all together.

**small** : 480x320 viewport size

**low** : reduced shadow map size; only characters cast shadows

**vl** : vertex lighting mode

Other launch options can be configured through [opt.js](https://github.com/glebnovodran/ostinato/blob/main/web/opt.js) 

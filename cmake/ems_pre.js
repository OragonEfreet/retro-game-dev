// Ensures the .wasm file is fetched relative to the generated .js file.
// (Useful when serving index.html and the game artifacts from the same folder.)
if (typeof Module === 'undefined') Module = {};
if (!Module.locateFile) {
  Module.locateFile = (path) => path;
}

Original repository: https://github.com/TheQwertiest/foo_spider_monkey_panel

Original author: https://github.com/TheQwertiest

Docs: https://theqwertiest.github.io/foo_spider_monkey_panel/

# Requirements
Because new methods from the `foobar2000` `SDK` are being used, this component
requires at least `foobar2000` `2.0`.

# Changelog

## 1.6.2.25.09.13
- Add `FbMetadbHandleList` `OptimiseFileLayout`. It takes a `boolean` `minimise`
argument. This provides the same functionality as the native context menu items
under `Utilities` except there are no prompts.
- Suppress all library callbacks on startup until it is fully initialised. All
library items will be passed to `on_library_items_added` which existing scripts
already expect.

## 1.6.2.25.09.10
- Fix bug where the `on_key_up` callback was not triggered for certain keys.
- Fix missing auto-complete entry for `on_download_file_done`.

## 1.6.2.25.09.06
- Various internal improvements.
- Minor sample fixes.

## 1.6.2.25.07.31
Update `utils.HTTPRequestAsync` to use a thread pool which limits concurrent
requests.

## 1.6.2.25.07.22
- The `type` values supplied to the following methods are now validated and will
throw errors if invalid.

```js
window.GetColourCUI
window.GetColourDUI
window.GetFontCUI
window.GetFontDUI
```
- Add [build](BUILDING.md) instructions.

## 1.6.2.25.06.21
- Add utils.HTTPRequestAsync. See [web requests](web-requests.md).

## 1.6.2.25.06.17
- Various bug fixes.

## 1.6.2.25.06.09
- Fix crash with original `plman.AddLocations` array parser that was recently
reused in the new `fb.AddLocationsAsync` method. Now a script error is thrown. Note
that array elements are not validated and each one is treated as a string.
- `plman.AddLocations` now throws a script error if a playlist lock is in
place that prevents adding of new items.

## 1.6.2.25.05.27
- Fix includes and package imports for paths containing non-ascii chars.
- Lots of internal refactoring around image, album art and text file
handling. Please report any issues if you find them.

## 1.6.2.25.05.23
- Fix regression affecting JSON property exports.

- Add `fb.CustomVolume` property. It can be used for displaying the volume from `UPnP` devices. It
will return a value of `-1` when using a normal device and that also indicates that `fb.Volume` is
writable. When a custom volume control is active, you can not use `fb.Volume` and must use
`fb.VolumeUp()` / `fb.VolumeDown()` / `fb.VolumeMute()`.

- Add `window.DPI`. This value never changes while `foobar2000` is running. If you change `DPI`
settings, you must restart the application.

- Remove all `Smooth` samples and `JSPlaylist-mod`. They are in an appalling state
and I refuse to distribute or be responsible for them.

## 1.6.2.25.05.20
- Update to `Spider Monkey` `91.10.0esr`.
- Fix `utils.Glob` so it works with `;` in folder names.
- Add `utils.DownloadFileAsync(url, path)` and `on_download_file_done` callback. 

The parent folder for `path` must already exist.

Example:

```js
// no return value, the path you supply is the first argument in the
// on_download_file_done callback
utils.DownloadFileAsync(
    "https://lastfm.freetls.fastly.net/i/u/770x0/0be145cbf80930684d41ad524fe53768.jpg",
    "z:\\blah.jpg"
);

// success is a boolean value
// error_text is always empty if success was true
function on_download_file_done(path, success, error_text) {
	console.log("on_download_file_done", path, success, error_text);
}
```

- Update included `Thumbs` sample to use the new method above.
- Smooth wallpaper fixes.
- Various other sample fixes.

## 1.6.2.25.05.19
- Add `window.IsDark` boolean property. The `on_colours_changed` callback has always
responded to dark mode being toggled.
- Ensure `fb.IsMainMenuCommandChecked` is always reliable.

## 1.6.2.25.05.14
- Add `fb.AddLocationsAsync(locations)` and `on_locations_added` callback.

`locations` must be an array of strings and it can contain file paths, playlists or urls.

Example:

```js
function on_mouse_lbtn_dblclk() {
	var files = ["z:\\1.mp3", "z:\\2.flac"];
	var task_id = fb.AddLocationsAsync(files);
	console.log("got task_id", task_id);
}

function on_locations_added(task_id, handle_list) {
	console.log("callback task_id", task_id);
	console.log(handle_list.Count);
}
```

## 1.6.2.25.05.13
- Add `FbMetadbHandleList` `SaveAs`. Saves using native `.fpl`
format so you should use that as the file extension. The
parent folder must already exist. No return value and if
it fails, it happens silently.

- Add `fb.ShowPictureViewer(image_path)`. This uses the image
viewer built in to `foobar2000`. No return value.

## 1.6.2.25.05.10
- `foobar2000` `2.25` preview has changed the behaviour of `FbMetadbHandle` `RawPath` if you
have a portable install and music files on the same drive. Any code that checks `startsWith("file://")`
will fail because the `RawPath` now starts with `file-relative://`. This release restores the old
behaviour.

## 1.6.2.25.05.05
- Add `fb.GetAudioChunk` / `FbAudioChunk` interface. See `vu meter` sample.
- Add `utils.GetClipboardText`. Returns an empty string if the clipboard contents
are not text.
- Add `utils.SetClipboardText`. No return value.
- Add `plman.GetGUID` / `plman.FindByGUID`.

A `GUID` is a unique identifier that persists with the playlist for its entire
lifetime between restarts and being renamed.

```js
var guid_str = plman.GetGUID(playlistIndex); // throws an error if playlistIndex is out of bounds
var playlistIndex = plman.FindByGUID(guid_str); // returns playlistIndex or -1 if not found
```

- Fix `utils.ColourPicker` bugs

- Fix `utils.IsFile` / `utils.IsDirectory` bugs.

- Update various samples fixed by `regor`.

- The `on_library_items_changed` callback now has a secondary `fromhook` argument so you can ignore updates that are not tag edits but database updates from components like `foo_playcount`.

```js
function on_library_items_changed(handles, fromhook) {
    if (fromhook)
        return;
   // react to actual file tag changes here
}
```

- Various improvements to handle list iteration methods.

- Various other bug fixes.

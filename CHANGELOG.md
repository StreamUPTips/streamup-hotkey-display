# OBS Hotkey Display - Changelog

---

## v1.0.0 (25 Oct '25)
**Patch Focus:** Update to OBS 32
- Added logging functionality
- Fixed Shift + F key combinations not being detected correctly
- Added mouse click detection
- Added required modifier key for mouse actions
- Update to OBS 32
- Refined the dock layout spacing so the hotkey display matches OBS theming margins
- Added localisation

## v0.0.1 (10 Sept '24)
**Initial Release**
Core functionality for displaying hotkeys in OBS:
- Live hotkey display dock with real-time keyboard hook functionality
- OBS dock integration with start/stop controls
- Text source integration with prefix and suffix support
- Scene name display settings
- Timer settings for hotkey display duration
- Websocket support for broadcasting key press events to external applications
- Localization framework for translations
- Cross-platform support (Windows, Mac, Linux)
- Hook state persistence across OBS sessions
- Settings menu with customizable display options
- Fixed high CPU usage issues
- Fixed startup UI errors

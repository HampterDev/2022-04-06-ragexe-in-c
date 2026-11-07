<p align="center">
  <img src="res/icon.png" width="112" alt="Ragnarok Online icon">
</p>

<h1 align="center">2022-04-06 Ragexe in C</h1>

<p align="center">
  <img src="https://img.shields.io/badge/client-Ragexe_2022--04--06-1f6feb" alt="client: Ragexe 2022-04-06">
  <img src="https://img.shields.io/badge/sha256-656b2f274c09-444" alt="original exe sha256 656b2f274c09">
  <img src="https://img.shields.io/badge/md5-f914d47b44f1-444" alt="original exe md5 f914d47b44f1">
  <img src="https://img.shields.io/badge/language-C11-00599C" alt="language: C11">
  <img src="https://img.shields.io/badge/platform-Windows_x86-0078D6" alt="platform: Windows x86">
  <img src="https://img.shields.io/badge/renderer-Direct3D_9-6e40c9" alt="renderer: Direct3D 9">
  <img src="https://img.shields.io/badge/exe-4.5_MB_(original_11.6_MB)-2ea44f" alt="exe 4.5 MB, original 11.6 MB">
</p>

The Korean Ragnarok Online client of 6 April 2022 (`Ragexe`), reconstructed from its
executable and rebuilt as a plain C source tree: the sources the client is built from, the
third-party libraries it links, and its behavioral tests. It builds with the MSVC C
compiler and linker for 32-bit Windows (Visual Studio 2022 or its Build Tools, x86).

Ragexe 2022-04-06 is the client version the BrowEdit community recommends most, for its
stability.

## Build

From cmd or PowerShell in this folder:

    build.bat          build\main_rebuild.exe
    build.bat dbg      build\main_dbg.exe with build\main_dbg.pdb

`build.bat` builds the libraries in `vendor\` first (into `build\vendor\`), then compiles
every `.c` under `src\` and links. There are no configuration switches: the client is
the Direct3D 9 build with the crash report, the texture cache lock and the Lua
compatibility layer.

## Tests

    tests\build.bat

builds and runs every behavioral test (about 15 minutes); each test prints a line that
ends in "passed". It builds the libraries in `vendor\` first if `build.bat` has not. A single
area has its own `tests\build_<name>.bat`; run `build.bat` or `tests\build.bat` once before
it, for the libraries. The tests need nothing outside this folder: no game data, server,
account or registry settings.

## Layout

| folder | what |
|---|---|
| `src\base.h` | the base header every unit includes |
| `src\core` | start-up, the message pump, configuration, text, files, Lua |
| `src\login`, `src\game`, `src\world`, `src\session` | the login screens, the game mode, the 3D world, the player's session state |
| `src\initializers`, `src\iteminfo`, `src\json`, `src\replay`, `src\browser` | static tables, item data, JSON, replays, the web browser window |
| `src\actor`, `src\skills`, `src\effects` | characters and monsters, skill logic, visual effects |
| `src\ui` | windows and controls |
| `src\render` | the Direct3D 9 renderer, textures, fonts, sprites |
| `src\resources`, `src\fileio` | game resource formats, GRF archives, the resource cache |
| `src\contents`, `src\web`, `src\navi` | game systems, web services, navigation |
| `src\std` | containers and strings |
| `src\platform`, `src\security`, `src\diag` | Windows integration, checks, the crash report and the in-game overlay |
| `vendor` | zlib, libpng, Lua 5.1.4, yyjson, Rijndael and the font backend (C++), curl headers |
| `lib`, `res` | prebuilt import libraries (Granny, Bink, Miles, IJL, ...), the compiled resources (`app.res`) and the client's icon (`app.ico`, `icon.png`) |
| `tests` | the behavioral tests and their fixtures |

## Conventions

- C11, the MSVC compiler; one declaration per function and object, in its header.
- No comments in the sources; names say what things are.
- Nothing that the client does not use: a function, global, type or macro without a user
  is removed.
- Third-party code in `vendor\` stays as published, with its licence.

## Changes from the original client

This client is Ragexe 2022-04-06, the Korean Ragnarok Online client of 6 April 2022,
reconstructed from its executable and rebuilt as plain C. It is meant to behave like the
original exe. The differences below are deliberate. A change that a patch of the
[NEMO](https://gitlab.com/4144/Nemo) patcher also makes carries that patch's name, as NEMO
lists it, in an orange badge.

### Starting and connecting

- [![NEMO: HKLM To HKCU](https://img.shields.io/badge/NEMO-HKLM_To_HKCU-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/HKLMtoHKCU.qs)
  Settings are read from and written to `HKEY_CURRENT_USER` instead of `HKEY_LOCAL_MACHINE`,
  so the client needs no administrator rights. `Setup.exe` and most servers' setup tools still
  write `HKEY_LOCAL_MACHINE`, so the original's wait at start-up (run `Setup.exe` until the setup
  keys exist) could never end. Without the keys the client starts with the setup defaults
  instead: the primary display and Miles 2D positional sound.
- [![NEMO: Always read msgstringtable.txt](https://img.shields.io/badge/NEMO-Always_read_msgstringtable.txt-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/ReadMsgstringtabledottxt.qs)
  The message strings come from `msgStringTable.txt` for every service type. The file is
  required: the client has no built-in strings, and the kRO full client does not ship one, so
  it comes from a translation or the server's data (in `data\` or a GRF). Without it every
  label reads "NO MSG".
- [![NEMO: Force Send Client Hash Packet](https://img.shields.io/badge/NEMO-Force_Send_Client_Hash_Packet-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/ForceSendClientHash.qs)
  Packet 0x204 always carries the MD5 of the running exe. The original sent it only for some
  service and server types, with a fixed key or zeros in place of the digest.
- [![NEMO: Disable Game Guard (NProtect)](https://img.shields.io/badge/NEMO-Disable_Game_Guard_%28NProtect%29-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/DisableNProtect.qs)
  nProtect GameGuard is not started, its FindHack scan does not run, and the login and game
  loops ignore its error state. On an error the original disconnected, quit and showed an
  "nProtect GameGuard" message box. The GameGuard code itself is gone; a stub keeps its
  interface.
- [![NEMO: Disable 1rag1 type parameters](https://img.shields.io/badge/NEMO-Disable_1rag1_type_parameters-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/Disable1rag1Params.qs)
  The client starts without a launcher argument. The original refused to run unless the
  command line contained `1rag1`, `1sak1`, `1gld1`, `2rag2`, `1eve1` or `1pk1`.
- [![NEMO: Disable Ragexe Filename Check](https://img.shields.io/badge/NEMO-Disable_Ragexe_Filename_Check-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/DisableFilenameCheck.qs)
  The start-up process check never stops the client. The original quit with "Please run a
  genuine Ragnarok client".
- [![NEMO: Remove hardcoded address/port](https://img.shields.io/badge/NEMO-Remove_hardcoded_address%2Fport-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/RemoveHardcodedAddress.qs)
  The login server is `<address>:<port>` from `clientinfo.xml`. The original picked a
  built-in `*.ragnarok.co.kr` server by server type, and set the client version and the
  built-in admin lists with it.
- [![NEMO: Restore old login packet](https://img.shields.io/badge/NEMO-Restore_old_login_packet-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/RestoreOldLoginPacket.qs)
  Every service type logs in with packet 0x64 (account and password). For korea (langtype
  0), brazil (12) and france (18) the original sent the token login 0x825, with the token
  from the login server's reply in place of the password, which an emulator cannot check
  against the account.
- The client does not look for a newer patcher at start. When `Patchup.exe` (`Sakup.exe`,
  `Indoorup.exe`, ... by server type) was next to it, the original asked "Ragnarok.exe is
  changed. Press ok to restart" and on OK replaced `Ragnarok.exe` with that file and ran it,
  which in a kRO full client folder starts the kRO patcher.
- The GDI check is retired. The original compared gdi32's `TextOutW`
  import with `GetProcAddress` and checked for a debugger. On a mismatch it disconnected and
  quit three times in five, and once due it ran every frame.

### Input and text

- [![NEMO: @ Bug Fix](https://img.shields.io/badge/NEMO-%40_Bug_Fix-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/FixChatAt.qs)
  While a text field has the keyboard, Ctrl+Alt (AltGr) keys type text instead of triggering
  hotkeys. AltGr+Q types `@` on a German layout; the original took it for Alt+Q (the
  equipment window) and lost the character. The same goes for `€` and `{ [ ] }`.

### WARP patches

The patches of a [WARP](https://github.com/Neo-Mind/WARP) session (PLV Online's
`warpsession.yml`) are made here in the source; each carries its WARP name in a blue badge.
Where WARP changes a copy of the code that the 2022 exe never calls (DecrAFDelay's follow
delay, NoHourly's Game Grade badge, EnableWho's plain user count), the patched exe does not
change, and neither does this client.

Starting and connecting:

- [![WARP: NoMultiWindow](https://img.shields.io/badge/WARP-NoMultiWindow-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoMultiWindow.qjs)
  A second client started while one is running exits at once (mutex `Global\Surface`).
- [![WARP: GRFsFromIni](https://img.shields.io/badge/WARP-GRFsFromIni-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/MultiGRFs.qjs)
  The GRF archives are `[Data]` keys 0-9 of `DATA.INI` beside the exe, key 0 searched first.
  Without any key the client writes `2=data.grf` into the file and loads that.
- [![WARP: CustomItemInfoLub](https://img.shields.io/badge/WARP-CustomItemInfoLub-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/CustomPath.qjs)
  Item data is read from `System\iteminfo.lua` instead of `System\iteminfo_true.lub`.
- [![WARP: NoMapInfo](https://img.shields.io/badge/WARP-NoMapInfo-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoMapInfo.qjs)
  `system\mapInfo_true.lub` is not loaded, so no map sign or banner is shown on entering a
  map. Map names from `mapNameTable.txt` still work.
- [![WARP: CallKoreaClientInfo](https://img.shields.io/badge/WARP-CallKoreaClientInfo-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/CallKoreaClientInfo.qjs)
  Every service type takes the Korean client-info path, so the 0xacf login step always
  connects to `<address>:<port>` from `clientinfo.xml`, also when `/account:` names another
  config file.
- [![WARP: EnableDnsSupport](https://img.shields.io/badge/WARP-EnableDnsSupport-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/EnableDnsSupport.qjs)
  `<address>` in `clientinfo.xml` may be a host name; it is resolved when a connection is
  read and once at network start-up.
- [![WARP: NoNagle](https://img.shields.io/badge/WARP-NoNagle-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoNagle.qjs)
  The server connection uses `TCP_NODELAY`: packets are sent at once instead of being batched.
- [![WARP: SendClientFlags](https://img.shields.io/badge/WARP-SendClientFlags-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/SendClientFlags.qjs)
  Login packets 0x64, 0x1dd, 0x1fa and 0x27c carry 0x80000001 in their version field instead
  of `<version>` from `clientinfo.xml`, which is no longer read. Bit 0 tells the server the
  cash shop can preview equipment (PreviewInShop).
- [![WARP: HideLicense](https://img.shields.io/badge/WARP-HideLicense-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/LicenseScreenVis.qjs)
  The login starts at service selection; the warning notice and the licence screen are
  skipped and `..\licence.txt` is not read.
- [![WARP: OpenToServiceSelect](https://img.shields.io/badge/WARP-OpenToServiceSelect-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/OpenToServiceSelect.qjs)
  The login window's "Opening" button goes back to service selection.
- [![WARP: NoGNJoyLaunch](https://img.shields.io/badge/WARP-NoGNJoyLaunch-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoGNJoyLaunch.qjs)
  The client opens no official Ragnarok web page: not the kRO page on auth result 900, the
  India homepage on quit, the iRO and Kunlun account pages, or the Hangame/kRO page after
  three wrong passwords. The message boxes and quits on those paths stay.
- [![WARP: DisConnToLogin](https://img.shields.io/badge/WARP-DisConnToLogin-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/ReturnToLogin.qjs)
  After a disconnect or ban notice from the map server the client shows the message and
  returns to the login screen instead of closing.
- [![WARP: NoUsernameLimit](https://img.shields.io/badge/WARP-NoUsernameLimit-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoLetterLimit.qjs) [![WARP: NoPasswordLimit](https://img.shields.io/badge/WARP-NoPasswordLimit-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoLetterLimit.qjs)
  The login window accepts account names shorter than 4 characters and passwords shorter than
  6.
- [![WARP: HideBuildInfo](https://img.shields.io/badge/WARP-HideBuildInfo-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Patches/Data.yml)
  The exe carries no build date: the crash report's "Build:" line shows only the image stamp,
  which `/Brepro` makes a content hash, and the release has no debug directory or PDB path.
- [![WARP: NoGGuard](https://img.shields.io/badge/WARP-NoGGuard-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/DisableProtect.qjs) [![WARP: No1and1Arg](https://img.shields.io/badge/WARP-No1and1Arg-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/ClientArgs.qjs) [![WARP: NoFilenameCheck](https://img.shields.io/badge/WARP-NoFilenameCheck-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoFilenameCheck.qjs) [![WARP: NoHardCodedAddr](https://img.shields.io/badge/WARP-NoHardCodedAddr-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoHardCodedAddr.qjs) [![WARP: UseOldLogin](https://img.shields.io/badge/WARP-UseOldLogin-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/LoginMode.qjs) [![WARP: MsgStrings](https://img.shields.io/badge/WARP-MsgStrings-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/ReadTxtFile.qjs)
  These are the NEMO changes listed above: no GameGuard, no launcher argument, no filename
  check, the login server from `clientinfo.xml`, login packet 0x64 and `msgStringTable.txt`
  for every service type.

Window, display and sound:

- [![WARP: CustomWinTitle](https://img.shields.io/badge/WARP-CustomWinTitle-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/WindowFrame.qjs)
  The game window is titled "PLV Online"; the same text is its window class name and the
  in-game browser's title.
- [![WARP: EnableSysMenu](https://img.shields.io/badge/WARP-EnableSysMenu-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/WindowFrame.qjs)
  The game window has a system menu with its icon, minimize and close buttons, and Alt+Space,
  also after switching back from fullscreen or borderless.
- [![WARP: CustomFontCellHeight](https://img.shields.io/badge/WARP-CustomFontCellHeight-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/CustomFont.qjs)
  UI text has a 14 px cell height whatever size the UI asks for, as in PLV Online's exe. The
  font face and weight stay each language's: warpsession.yml also lists CustomFontName ("Eras
  Medium ITC"), CustomFontHeight (12) and CustomFontWeight (400), but the exe PLV ships has none
  of them. The Rix and SCDream fonts are unchanged.
- [![WARP: IncrMapQuality](https://img.shields.io/badge/WARP-IncrMapQuality-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/IncrMapQuality.qjs)
  Map lightmaps are 32-bit textures instead of 16-bit 4444, so ground lighting has no banding.
- [![WARP: RestoreModelCulling](https://img.shields.io/badge/WARP-RestoreModelCulling-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/RestoreModelCulling.qjs)
  Map models between the camera and the player turn translucent and fade back when they no
  longer block the view.
- [![WARP: MediumCamAngle](https://img.shields.io/badge/WARP-MediumCamAngle-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/IncrCamAngle.qjs)
  On free-camera maps, Shift + right-drag tilts the camera over a pitch range of 42 instead
  of 20 (about -87 to -28 degrees, -3 with Ctrl, instead of -65 to -50).
- [![WARP: ZoomMax](https://img.shields.io/badge/WARP-ZoomMax-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/IncrZoom.qjs)
  The camera zooms out to a distance of 1224 instead of 400 (480 with `/zoom`); `/zoom` and
  its checkbox still toggle and save the setting but no longer change the limit.
- [![WARP: NoGravityAds](https://img.shields.io/badge/WARP-NoGravityAds-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoGravityImages.qjs) [![WARP: NoGravityLogo](https://img.shields.io/badge/WARP-NoGravityLogo-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoGravityImages.qjs)
  The login screen shows no Gravity logo, age-rating image, tape strips or Game Grade badge.
- [![WARP: NoSerialDisplay](https://img.shields.io/badge/WARP-NoSerialDisplay-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoSerialDisplay.qjs)
  The login screens no longer show the serial placeholder at the bottom right.
- [![WARP: CustomIcon](https://img.shields.io/badge/WARP-CustomIcon-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Patches/Env.yml)
  The exe's icon and the window's icon are PLV Online's 32x32 `ragnarok.ico`: icon groups 114
  and 119 in `res\app.res` hold it in place of the generic and the 32x32 Ragnarok icon. Group
  123, `app.ico` and `icon.png` keep the Ragnarok icon.
- [![WARP: FixLatestNCWin](https://img.shields.io/badge/WARP-FixLatestNCWin-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/FixLatestNCWin.qjs)
  The new character-making screen has no `bg_back2.tga` backdrop and its window sits 64 px
  lower.
- [![WARP: Enable44kHzAudio](https://img.shields.io/badge/WARP-Enable44kHzAudio-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/CustomSound.qjs)
  Sound plays at 44,100 Hz instead of 22,050 Hz for the default rate setting
  (`DIGITALRATETYPE` 0); types 1 and 2 stay 11,025 and 8,000 Hz.
- [![WARP: AutoMute](https://img.shields.io/badge/WARP-AutoMute-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/CustomSound.qjs)
  In game the client mutes music and effects within half a second of losing focus and
  restores the volumes when it is active again. Options saved while muted save an effect
  volume of 0.
- [![WARP: TranslateClient](https://img.shields.io/badge/WARP-TranslateClient-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/TranslateClient.qjs)
  The client's own interface texts are English, as `Translations_EN.yml` makes them:
  "Message" box titles, "Exit", "Cart Off", "No Color", "Total : %d Cash", "%dea" and the AI
  trace timestamp. The file's other entries translate the exe's built-in Korean message
  table, which this client does not have; their English comes from `msgStringTable.txt`.

Game and chat:

- [![WARP: NoGMSprite](https://img.shields.io/badge/WARP-NoGMSprite-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoGMSprite.qjs)
  GM accounts are drawn with their own body and weapon sprites instead of the GM sprite; the
  yellow name and GM menus stay.
- [![WARP: AllowSkillSpam](https://img.shields.io/badge/WARP-AllowSkillSpam-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/AllowSkillSpam.qjs)
  Holding a skill hotkey keeps casting the skill; the original ignored auto-repeated key
  presses.
- [![WARP: CustomWalkDelay](https://img.shields.io/badge/WARP-CustomWalkDelay-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/WalkDelay.qjs)
  A held left click repeats the walk request without the 600 ms and 350 ms waits.
- [![WARP: CustomFadeDelay](https://img.shields.io/badge/WARP-CustomFadeDelay-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/CustomFadeDelay.qjs)
  Warps and mode changes switch the screen without the black fade-out and fade-in; the music
  still fades.
- [![WARP: UnlimitedChatRepeat](https://img.shields.io/badge/WARP-UnlimitedChatRepeat-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/ChatRepeat.qjs)
  The same chat line can be sent any number of times in a row; the original refused the third
  (message 0x31).
- [![WARP: NoSwearFilter](https://img.shields.io/badge/WARP-NoSwearFilter-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoSwearFilter.qjs)
  `manner.txt` is not loaded, so no chat, name or title is censored.
- [![WARP: GuildBrackets](https://img.shields.io/badge/WARP-GuildBrackets-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/GuildBrackets.qjs)
  For service type 0 name balloons show party and guild as `name (party) ` instead of
  `name『party』`.
- [![WARP: NoHourly](https://img.shields.io/badge/WARP-NoHourly-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoHourly.qjs)
  No "N hours have passed" play-time reminder.
- [![WARP: NoHelpMsg](https://img.shields.io/badge/WARP-NoHelpMsg-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoHelpMsg.qjs)
  Entering the game no longer prints the help text or, for service type 0, requests the
  remaining play time (0x1c0).
- [![WARP: EnableWho](https://img.shields.io/badge/WARP-EnableWho-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/EnableSlashCmd.qjs)
  `/w` and `/who` request the user count for every service type; the original did not for 3,
  8 and 9.
- [![WARP: EnableShowName](https://img.shields.io/badge/WARP-EnableShowName-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/EnableSlashCmd.qjs)
  `/showname` and its saved setting switch the name font for every service type, not only 0,
  6, 0xa and 0xc.
- [![WARP: AlwaysAscii](https://img.shields.io/badge/WARP-AlwaysAscii-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/AlwaysAscii.qjs)
  On service type 1 character names, chat room titles and chat, room and whisper lines with
  non-ASCII characters are no longer refused.
- [![WARP: NoCharnameLimit](https://img.shields.io/badge/WARP-NoCharnameLimit-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoLetterLimit.qjs)
  Character names may be shorter than 4 bytes in both character creation windows; as in the
  patched exe, RODEX mail titles may be too.
- [![WARP: ResizeChatBox](https://img.shields.io/badge/WARP-ResizeChatBox-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/ResizeBox.qjs) [![WARP: ResizeChatRoomBox](https://img.shields.io/badge/WARP-ResizeChatRoomBox-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/ResizeBox.qjs) [![WARP: ResizePMBox](https://img.shields.io/badge/WARP-ResizePMBox-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/ResizeBox.qjs)
  The chat line, the chat room's line and the whisper window's line take up to 255 characters
  instead of 70. The buffers they are sent through are sized for that.
- [![WARP: RestoreChatFocus](https://img.shields.io/badge/WARP-RestoreChatFocus-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/RestoreChatFocus.qjs)
  Clicking into the world no longer takes the keyboard away from the chat line.
- [![WARP: CloseCutinOnEsc](https://img.shields.io/badge/WARP-CloseCutinOnEsc-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/CloseCutinOnEsc.qjs)
  Esc closes an open cutin picture. As in the patched exe, the same press then goes on to
  close the topmost window or open the Esc menu.
- [![WARP: FixChatAt](https://img.shields.io/badge/WARP-FixChatAt-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/FixChatAt.qjs)
  Covered by the "@ Bug Fix" above, without WARP's side effect of also firing the Alt hotkey.

Windows and shops:

- [![WARP: ResizeNormalBar](https://img.shields.io/badge/WARP-ResizeNormalBar-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/ResizeMobBar.qjs) [![WARP: ResizeBossBar](https://img.shields.io/badge/WARP-ResizeBossBar-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/ResizeMobBar.qjs)
  The HP bar over normal monsters is 90x5 and over bosses 120x5 instead of 60x5; mini-bosses
  keep 60x5.
- [![WARP: ShowExpNumbers](https://img.shields.io/badge/WARP-ShowExpNumbers-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/CustomExpBars.qjs)
  The expanded Basic Info window shows base and job experience as "current / next" in light
  green (RGB 0x9a, 0xd4, 0x93) at the bars.
- [![WARP: CustomPartyLimit](https://img.shields.io/badge/WARP-CustomPartyLimit-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/CustomFriendWin.qjs)
  The party tab shows the member count out of 5 instead of 12.
- [![WARP: MoveUpItemCount](https://img.shields.io/badge/WARP-MoveUpItemCount-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/MoveUpItemCount.qjs)
  Item counts on the shortcut bar are drawn higher and outlined, like the skill levels.
- [![WARP: PlainTextDesc](https://img.shields.io/badge/WARP-PlainTextDesc-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/PlainTextDesc.qjs)
  Text data is read as stored for every service type: the Korean letter mirroring of item
  descriptions, the talk tables, the monster skill table and the scream lines is no longer
  undone.
- [![WARP: ShortcutForAll](https://img.shields.io/badge/WARP-ShortcutForAll-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/ShortcutForAll.qjs)
  Any identified inventory item of type 0-0x13 can go on the shortcut bar and the quick slots.
- [![WARP: HideNewButtons](https://img.shields.io/badge/WARP-HideNewButtons-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NewButtonVisibility.qjs)
  The menu icon window has no Guild, World Map, Navigation, Bank, Record, Tip, Adventurer
  Agency or Reputation button; the others move up.
- [![WARP: HideTraitStatusButton](https://img.shields.io/badge/WARP-HideTraitStatusButton-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/TraitStatusButton.qjs) [![WARP: DisableTraitStatusButton](https://img.shields.io/badge/WARP-DisableTraitStatusButton-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/TraitStatusButton.qjs)
  The status window has no trait expand button and keeps the size it is restored with. The
  trait "+" buttons still work; in the patched exe the disable jump also swallowed them.
- [![WARP: NoAdvAgencyInParty](https://img.shields.io/badge/WARP-NoAdvAgencyInParty-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/NoAdvAgencyInParty.qjs)
  The party tab's Adventurer Agency button stays greyed out.
- [![WARP: ShowResButton](https://img.shields.io/badge/WARP-ShowResButton-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/ResButtonVis.qjs)
  The Resurrection button also shows on PvP, GvG/siege and event-PvP maps.
- [![WARP: ShowRoulette](https://img.shields.io/badge/WARP-ShowRoulette-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/RouletteVis.qjs)
  The Roulette icon is created on every map entry, as in older clients; its show/hide setting
  still applies.
- [![WARP: DefShopBrowser](https://img.shields.io/badge/WARP-DefShopBrowser-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/CashShop.qjs)
  The cash shop banner and charge button and the attendance banner open in the default
  browser instead of `iexplore.exe`.
- [![WARP: PreviewInShop](https://img.shields.io/badge/WARP-PreviewInShop-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/PreviewInShop.qjs)
  Cash shop item lists (0x8ca) use 14-byte records with the view sprite and equip location,
  so the cash shop can preview equipment. The server must send that format.
- [![WARP: InsensitiveStorageSearch](https://img.shields.io/badge/WARP-InsensitiveStorageSearch-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/CaseInsensitiveSearch.qjs) [![WARP: InsensitiveShopSearch](https://img.shields.io/badge/WARP-InsensitiveShopSearch-2980b9)](https://github.com/Neo-Mind/WARP/blob/rock_win32/Scripts/Patches/CaseInsensitiveSearch.qjs)
  The storage and cash shop item searches ignore upper and lower case.
### Bugs of the original fixed

Dangling pointers and races:

- A window that is destroyed is removed from everything that
  points at it: the window manager's slots, the modal, capture, focus and hover pointers, and
  the windows actors float above their heads. The original cleared these by hand at each
  close site, so a window destroyed any other way (a mode change, a parent closing its
  children) left dangling pointers.
- The blur and hallucination clones check that the actor they copy is
  still in the world before they read it. The original read freed memory when that actor was
  deleted before its clone's tick.
- Hiding the UI (F11) no longer leaves the player's status-icon
  effect pointer dangling. With the UI hidden, using an item from a hotkey read freed memory.

Uninitialized values:

- Without a saved `CURSLOT` registry value, character selection starts at the first slot
  instead of selecting a slot from leftover stack data and potentially disabling Game Start.
- The damage numbers of one hit packet came out blue at random. Its
  "SP damage" byte was left as the stack had it.
- The key-stroke window read its colour chip's green from the stack.
  Green and blue went into one variable.
- Achievement default records, the tetra-ball and flare effect
  positions, world-map party marks and radio-button commands now start with defined values
  instead of leftover memory.
- A button in an unknown state draws no
  bitmap. An unknown on-screen timer kind is hidden; the original showed a pointer as the
  timer's value.
- An STR texture animation playing in reverse wraps to the end of its textures when it
  passes frame 0. The original took the wrap from an uninitialised value, so the layer stuck
  on its first texture or showed the white one (kRO `hogg.str`, `ssnnnn2.str`,
  `fullmoon_kick.str`).

Overruns and bad table indexes:

- The cash shop window's button loop ran a twentieth time, past its
  19-entry tables. It took stack data as bitmap names, a likely crash.
- The length of NPC dialog text from the server is clamped to the
  buffer (0..0x7fd), and incomplete packets are rejected.
- A long item name can no longer overflow the file path of the
  package item test report.
- Negative angles are normalised (-90 is 270). The original indexed
  before its sine and cosine tables.
- Bytes above 0x7f no longer index before the hex digit table, and
  non-hex characters decode as 0.
- The font lookup releases the screen DCs it takes; the original
  leaked two per lookup. An unknown font id, SCDream's fallback included, uses GDI's default
  font instead of reading past the twelve font slots.

Format strings:

- The collection window copies the pet's name into its field. The
  original passed the name to `sprintf` as the format.
- Pet, homunculus and partner names from the server, the names typed in
  the pet and homunculus rename boxes and the default names taken from
  the job-name table are copied as text. The original passed each name to
  `sprintf` as the format, so a `%` in a name read arguments nobody passed.
- The renamed-pet prefix from the message table is printed as text, not
  used as a format; only `%%` still becomes `%`.
- The OTP text is sent as it is, not used as a `printf` format.
- The attendance error message prints the item id with `%d`. The
  original passed the id to a `%s`, which crashed on a duplicate reward.
- The equipment-swap tooltip for an item with random options gets
  the name, the option count and the amount. The original passed the name's `std::string`
  object itself.

Markup, links and wrapping:

- Colour codes (`^rrggbb`), markup tags
  and message tags are parsed within the text's length. An incomplete code or an unmatched tag
  stays as text. It no longer causes a loop, a read past the end, or a read before the font
  stack.
- Item and navigation links in chat and edit fields stay inside their text. At most 4 cards and 5
  random options are decoded. A malformed link cannot underflow an offset, rescan forever or
  delete the wrong text.
- When a link's button cannot be
  created, the text stays readable instead of a null pointer being used.
- Wrapping text into
  a box narrower than one character no longer loops forever. Every line takes at least one
  character.
- A description line that starts with one character and a `[...]` group that just misses the
  line no longer loops forever, and a step back before a group that does not fit moves one
  character, not one byte, so it no longer splits a double-byte character.
- Window text that breaks at a lone line feed or carriage return keeps the next line's first
  character. The original's line breaker kept the CR or LF in the line and skipped the byte
  after it, which is right only for CRLF text; for LF-only books and server texts it dropped
  the first letter of every line.

Data, time and leaks:

- A vending request keeps its setup items until the next setup starts. If the server
  refuses the request and leaves the window open, retrying still sends the chosen items,
  quantities and prices instead of an empty list.
- A reputation or EZ2STR effect table that is not a JSON object fails the
  read instead of throwing an exception out of the loader.
- GRF, ACT, SPR, IMF, GND, GAT, RSW, RSM and EZ effect files are read within their own size.
  A file cut short, or whose counts or indexes do not fit the file or the tables they point
  into, fails to load. The original read such a file as far as it went: its reads past the
  end failed and left zeros or earlier values, and the counts and offsets it took from the
  file read past the end or outside its tables. An ACT that ends inside its delay table,
  a PAL or SPR too short for its palette and an RSM model that ends before or inside its
  volume boxes still load, as in the original. An EZ effect file
  with a negative frame or texture count, or a 128-byte texture name with no NUL, fails as
  well. An RSM2 model of minor version 1 to 3 whose major version is not 2 fails; the
  original loaded it and then made the kind of model actor that major version names. A water
  animation speed of 0 fails the map; the original divided by it. A sprite frame whose pixel
  stream is shorter or longer than the frame is cut at the frame's end or filled with
  transparent pixels, and the sprite still loads. A GRF record that points outside the
  archive (the kRO `data.grf` has such a deleted-file record) is left out, and the other
  files stay readable.
- A GIF (a guild emblem or a `.gif` texture) is decoded as the original does, but never read
  or written past its buffers: bytes after the end of the file read as zero, a colour index
  past the palette as 0, an LZW stream stops at the end of its data, a code chain that loops
  skips its frame, and LZW table entries past 4096 or pixels past the frame are dropped. A GIF
  that would need more than 256 MB for its frames or for one image's pixels is refused instead
  of running out of memory.
- An empty XML element written without a space before the slash, such as `<readfolder/>`
  in `clientinfo.xml`, is read as empty. The original cut the tag name at the slash before
  checking for it, so the element swallowed the rest of the file and the connections in it.
- Player-name requests and the name cache still time out correctly
  when the millisecond clock wraps, after 49.7 days of Windows uptime.

### Graphics and display

- The renderer is Direct3D 9 (fixed function) instead of DirectX 7 (DirectDraw 7 and
  Direct3D 7); DirectDraw is no longer used. Vertex processing is in hardware, with software as
  the fallback. A lost device is reset and every state the client set is replayed.
- The Graphic Device list has one entry per display adapter, named after it ("... (display
  2)" for a second monitor), instead of DirectX 7's "Direct3D HAL" and "T&L HAL" pair.
- The resolution list takes modes up to 8192x4096 instead of 2048x1440, so
  wide desktops such as 3840x1080 appear. Each size is listed once (not once per refresh rate),
  and none is larger than the desktop.
- Exclusive fullscreen always uses a 32-bit back buffer; the 16-bit setting is not used.
- A new resolution and the Fullscreen checkbox in the graphics settings apply
  at once. The original asked for a restart and relaunched the exe. UI windows near the right
  or bottom edge keep their distance to that edge, centred ones stay centred, and the size is
  saved as before.
- Changing the Graphic Device applies at once when the new adapter has the same
  GPU model (same vendor and device id); textures are copied to the new device. Other device
  changes restart as before.
- A new "Borderless" checkbox, on by default and saved as `ISBORDERLESS` in `OptionInfo.lua`.
  With it on, Fullscreen is a frameless window covering the monitor at the monitor's own
  resolution: no mode switch, and no device loss on Alt+Tab. With it off, Fullscreen is the
  exclusive mode as in the original.
- The 3D scene is drawn with 4x multisampling; the original never anti-aliases.
  Flat 2D draws (the UI, sprites, the login screen) are drawn as before, without
  multisampling. Alpha-tested edges (fences, leaves) get the driver's transparency
  anti-aliasing where it is offered. `RAGEXE_MSAA=<n>` sets the sample count, and `0` turns it
  off. `RAGEXE_MSAA_ALPHA=0|ssaa|atoc` controls the transparency anti-aliasing.
- The wait for the vertical blank is the swap chain's, so `Present`
  blocks without spinning and the frame rate is the display's refresh rate.
  `RAGEXE_VSYNC=0` turns vsync off. The map-loading progress frames, which the original
  shows without waiting for the vertical blank, are presented without waiting too; one that
  finds the previous frame still queued is skipped.
- A device the driver reports as hung or removed, or one whose reset fails 8
  times in a row, is replaced by a new device on the same adapter, with the textures copied
  over. If the new device refuses the exclusive mode, the client falls back to windowed.
- A saved display device that is no longer attached (an unplugged
  monitor, a swapped GPU) is reset to the primary display in the registry, and a message box
  says so once.

### Performance

- Consecutive pre-transformed draws are merged into one indexed triangle list,
  in the original order. The `Sleep(0)` the original made after each such draw is gone.
- A render state, stage state or texture the
  device already holds is not sent again. The original re-sent most of them at every list
  flush.
- A blended draw with the same texture-stage mode as the previous one
  skips the mode switch.
- A sprite or UI texture rebuild that would upload the same pixels
  into an unchanged texture is skipped. In a loaded town that is nearly all of them.
- The frame no longer locks the render target for values nothing reads.
  The lock made the CPU wait for the GPU every frame.
- The game loop yields (`Sleep(0)`) after each frame instead of sleeping a
  millisecond. It still sleeps a millisecond while it draws nothing (alt-tabbed out of
  exclusive fullscreen), when no present waits for the display.
- Text measurements are cached, keyed on the font, the charset and the
  string.
- The lookup key prepared from a resource name is remembered, so per-frame
  fetches such as `shadow.spr` skip lower-casing and alias resolution. `RAGEXE_RES_KEYS=0`
  turns it off.
- Finding an actor by its id goes through an index instead of walking
  the world's actor list. Ids that are absent are remembered too.
- An actor's displayed job reads the effect-state mask once instead of
  once per rule (up to forty virtual calls).
- Effect particles reuse the camera angle's sine,
  cosine and rotation matrix while the camera does not turn. The results are the same as
  before, to the bit.
- UI windows the original redraws every frame (character selection, equipment, skill tree,
  dress room, cash shop, shortcut bar, quick slots, minimap buttons, vendor shop titles and
  the window of the focused edit line) redraw only in the frame where something they show
  changes: a drawn value, a hover, an animation frame, the caret blink. Cooldown pies and the
  characters on the selection screen are still drawn every frame over the unchanged window.
- The exe is large-address-aware: under 64-bit Windows it can use 4 GB of address space
  instead of 2 GB.

### Diagnostics built in

- An unhandled exception writes `crash_report.txt` (appended, at most 2 MB)
  and a minidump `crash-<date>-<time>-<pid>-<tid>-<n>.dmp` beside the exe. The report
  includes the exception, the address as an exe offset, memory and module lists, job, map and
  cell, the last effect ids and the render checkpoint. The original's error dialog then shows
  the report, and it can be copied from there.
- With `RAGEXE_OVERLAY=1` the client draws an FPS and frame-phase overlay
  (scene, present, logic, text, busiest render lists, draw and state counts). Scroll Lock
  toggles it in any session.
- With `RAGEXE_DRIVE_STATE=<file>` the client rewrites that file about four
  times a second with `key=value` lines. They hold the mode and screen, the map, the
  character's cell, HP, SP and level, the pickable actors with their screen boxes, the open
  and modal windows, the chat focus and the last chat lines. The file is for scripts that
  drive the client in tests.
- `RAGEXE_SWITCH_DEVICE=1` takes the device-switch path for every display change, to test it
  on a machine with one monitor.

### The C rewrite itself

Most of the rewrite is structural, with no intended change in behaviour. Objects have
ordinary C ownership instead of the MSVC C++ runtime's layouts: `std::string`, `vector`,
`list`, `map` and `deque` became `struct String`, `Vec`, `List` and `Tree`. Virtual tables
became operation tables, kept only where the implementation varies. The standard MSVC CRT
replaces the image's private runtime. `client_init()` builds the globals in the original
order instead of static constructors. yyjson replaces jsoncons. An unrecoverable error (out
of memory) prints a message to stderr and aborts instead of throwing a C++ exception.
Internal memory layouts are no longer the original's; packet, file and library layouts are.

Faults of the original found during the rewrite, now fixed:

- Player-only code wrote player fields into NPC actors (the hat effect, statuses 0x57, 0xeb
  and 0x3d6, the hallucination doubles). After-images of a caster that is not a player read
  past the object; they are now made only for players.
- The footprint effect launcher passed a float where a 3D vector is read.
- `CZ_ENTER`'s client tick field was sent with whatever was on the stack; it is now sent as 0.

## Contributing

The aim is a client that behaves exactly like the original exe, apart from the changes
listed above. If you find a reconstruction bug, anything this build does differently from
the original 2022-04-06 client, feel free to open a pull request (or an issue, if you have
not tracked it down yet). Please keep to the conventions above and run `tests\build.bat`
before you open it.

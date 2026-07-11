# Voyage ETA OpenCPN Plugin

Target:
- OpenCPN 5.14.x
- Windows x64
- OpenCPN Plugin API 1.18+ / host API 121 compatible

Features:
- Reads live own-ship latitude, longitude and SOG supplied by OpenCPN.
- Lists waypoints from the selected route.
- Calculates remaining route distance to a selected waypoint.
- Defaults to the last waypoint.
- Displays live SOG, distance remaining, TTG, UTC ETA and local ETA.
- Refreshes automatically as new navigation fixes arrive.

Important:
- The AIS pilot plug must send own-ship navigation data such as RMC or VTG.
- AIS target-only messages (`!AIVDM`) do not contain your own ship's SOG.
- Activate or load the required route in OpenCPN, open Voyage ETA, then select
  a route and waypoint.

## Build

This folder contains the plugin source. OpenCPN plugins require a Windows DLL
compiled against the OpenCPN/wxWidgets ABI. The included GitHub Actions workflow
builds the plugin using the OpenCPN testplugin template.

1. Create a new GitHub repository.
2. Upload all files from this folder.
3. Open the repository's **Actions** tab.
4. Run **Build Voyage ETA for Windows**.
5. Download the build artifact.
6. In OpenCPN use:
   `Options > Plugins > Import Plugin...`

The workflow may need a small template adjustment when OpenCPN changes its
official build image or template branch.

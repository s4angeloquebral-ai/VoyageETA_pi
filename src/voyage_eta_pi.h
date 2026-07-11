#pragma once

#include "ocpn_plugin.h"
#include <wx/timer.h>

class VoyageEtaDialog;

class VoyageEtaPlugin : public opencpn_plugin_118 {
public:
    explicit VoyageEtaPlugin(void* ppimgr);
    ~VoyageEtaPlugin() override;

    int Init() override;
    bool DeInit() override;

    int GetAPIVersionMajor() override { return 1; }
    int GetAPIVersionMinor() override { return 18; }
    int GetPlugInVersionMajor() override { return 1; }
    int GetPlugInVersionMinor() override { return 0; }

    wxBitmap* GetPlugInBitmap() override;
    wxString GetCommonName() override { return "Voyage ETA"; }
    wxString GetShortDescription() override {
        return "Live ETA to a selected route waypoint";
    }
    wxString GetLongDescription() override {
        return "Calculates live ETA using own-ship SOG and remaining route distance.";
    }

    void OnToolbarToolCallback(int id) override;
    void SetPositionFixEx(PlugIn_Position_Fix_Ex& pfix) override;

    double Latitude() const { return m_lat; }
    double Longitude() const { return m_lon; }
    double Sog() const { return m_sog; }
    bool HasFix() const { return m_hasFix; }

    void RefreshDialog();

private:
    VoyageEtaDialog* m_dialog = nullptr;
    int m_toolbarId = -1;
    double m_lat = 0.0;
    double m_lon = 0.0;
    double m_sog = 0.0;
    bool m_hasFix = false;
};

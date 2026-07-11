#pragma once

#include "ocpn_plugin.h"

#include <wx/wx.h>
#include <wx/choice.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>
#include <wx/timer.h>

#include <vector>

class VoyageEtaDialog;

class testplugin_pi : public opencpn_plugin_118 {
public:
    explicit testplugin_pi(void* ppimgr);
    ~testplugin_pi() override;

    int Init() override;
    bool DeInit() override;

    int GetAPIVersionMajor() override { return 1; }
    int GetAPIVersionMinor() override { return 18; }

    int GetPlugInVersionMajor() override { return 1; }
    int GetPlugInVersionMinor() override { return 0; }
    int GetPlugInVersionPatch() override { return 0; }
    int GetPlugInVersionPost() override { return 0; }

    wxString GetCommonName() override { return "Voyage ETA"; }
    wxString GetShortDescription() override {
        return "ETA to three selected route waypoints";
    }
    wxString GetLongDescription() override {
        return "Shows ETA to three selected OpenCPN route waypoints using live SOG or manual speed.";
    }

    wxBitmap* GetPlugInBitmap() override;
    int GetToolbarToolCount() override { return 1; }

    void OnToolbarToolCallback(int id) override;
    void SetPositionFixEx(PlugIn_Position_Fix_Ex& pfix) override;

    bool HasFix() const { return m_hasFix; }
    double Latitude() const { return m_lat; }
    double Longitude() const { return m_lon; }
    double Sog() const { return m_sog; }

private:
    VoyageEtaDialog* m_dialog = nullptr;
    int m_toolbarId = -1;
    wxBitmap m_icon;

    bool m_hasFix = false;
    double m_lat = 0.0;
    double m_lon = 0.0;
    double m_sog = 0.0;
};

struct VoyageRoutePoint {
    wxString name;
    double lat = 0.0;
    double lon = 0.0;
};

class VoyageEtaDialog : public wxDialog {
public:
    VoyageEtaDialog(wxWindow* parent, testplugin_pi* plugin);
    ~VoyageEtaDialog() override = default;

    void ReloadRoutes();
    void RefreshValues();

private:
    void OnRouteChanged(wxCommandEvent&);
    void OnWaypointChanged(wxCommandEvent&);
    void OnSpeedModeChanged(wxCommandEvent&);
    void OnManualSpeedChanged(wxCommandEvent&);
    void OnTimer(wxTimerEvent&);

    bool LoadSelectedRoute();
    double SelectedSpeed() const;
    double RemainingDistanceNm(size_t destinationIndex) const;

    static double DistanceNm(double lat1, double lon1, double lat2, double lon2);
    static wxString FormatEtaLocal(double hours);

    void UpdateEtaForChoice(wxChoice* choice, wxStaticText* etaLabel);

    testplugin_pi* m_plugin = nullptr;

    wxChoice* m_routeChoice = nullptr;

    wxRadioButton* m_useActual = nullptr;
    wxRadioButton* m_useManual = nullptr;
    wxTextCtrl* m_manualSpeed = nullptr;
    wxStaticText* m_actualSog = nullptr;

    wxChoice* m_waypointChoice1 = nullptr;
    wxChoice* m_waypointChoice2 = nullptr;
    wxChoice* m_waypointChoice3 = nullptr;

    wxStaticText* m_eta1 = nullptr;
    wxStaticText* m_eta2 = nullptr;
    wxStaticText* m_eta3 = nullptr;

    wxStaticText* m_status = nullptr;

    wxArrayString m_routeGuids;
    std::vector<VoyageRoutePoint> m_points;
    wxTimer m_timer;

    wxDECLARE_EVENT_TABLE();
};

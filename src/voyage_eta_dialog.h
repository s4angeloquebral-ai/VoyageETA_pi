#pragma once

#include <wx/wx.h>
#include <wx/choice.h>
#include <wx/timer.h>
#include <vector>

class VoyageEtaPlugin;

struct RoutePointLite {
    wxString name;
    double lat = 0.0;
    double lon = 0.0;
};

class VoyageEtaDialog : public wxDialog {
public:
    VoyageEtaDialog(wxWindow* parent, VoyageEtaPlugin* plugin);

    void ReloadRoutes();
    void RefreshValues();

private:
    void OnRouteChanged(wxCommandEvent&);
    void OnWaypointChanged(wxCommandEvent&);
    void OnRefresh(wxCommandEvent&);
    void OnTimer(wxTimerEvent&);

    bool LoadSelectedRoute();
    double RemainingDistanceNm(size_t destinationIndex) const;
    static double DistanceNm(double lat1, double lon1, double lat2, double lon2);
    static wxString FormatDuration(double hours);
    static wxString FormatEta(const wxDateTime& dt);

    VoyageEtaPlugin* m_plugin = nullptr;
    wxChoice* m_routeChoice = nullptr;
    wxChoice* m_waypointChoice = nullptr;
    wxStaticText* m_sogValue = nullptr;
    wxStaticText* m_distanceValue = nullptr;
    wxStaticText* m_ttgValue = nullptr;
    wxStaticText* m_etaUtcValue = nullptr;
    wxStaticText* m_etaLocalValue = nullptr;
    wxStaticText* m_statusValue = nullptr;
    wxTimer m_timer;

    wxArrayString m_routeGuids;
    std::vector<RoutePointLite> m_points;

    wxDECLARE_EVENT_TABLE();
};

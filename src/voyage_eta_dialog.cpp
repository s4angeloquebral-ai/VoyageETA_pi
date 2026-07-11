#include "voyage_eta_dialog.h"
#include "voyage_eta_pi.h"
#include "ocpn_plugin.h"
#include <cmath>
#include <limits>

enum {
    ID_ROUTE = wxID_HIGHEST + 310,
    ID_WAYPOINT,
    ID_REFRESH,
    ID_TIMER
};

wxBEGIN_EVENT_TABLE(VoyageEtaDialog, wxDialog)
    EVT_CHOICE(ID_ROUTE, VoyageEtaDialog::OnRouteChanged)
    EVT_CHOICE(ID_WAYPOINT, VoyageEtaDialog::OnWaypointChanged)
    EVT_BUTTON(ID_REFRESH, VoyageEtaDialog::OnRefresh)
    EVT_TIMER(ID_TIMER, VoyageEtaDialog::OnTimer)
wxEND_EVENT_TABLE()

VoyageEtaDialog::VoyageEtaDialog(wxWindow* parent, VoyageEtaPlugin* plugin)
    : wxDialog(parent, wxID_ANY, "Voyage ETA",
               wxDefaultPosition, wxSize(460, 430),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_plugin(plugin),
      m_timer(this, ID_TIMER) {

    auto* top = new wxBoxSizer(wxVERTICAL);

    auto* grid = new wxFlexGridSizer(2, 8, 10);
    grid->AddGrowableCol(1, 1);

    grid->Add(new wxStaticText(this, wxID_ANY, "Route:"), 0, wxALIGN_CENTER_VERTICAL);
    m_routeChoice = new wxChoice(this, ID_ROUTE);
    grid->Add(m_routeChoice, 1, wxEXPAND);

    grid->Add(new wxStaticText(this, wxID_ANY, "Destination waypoint:"),
              0, wxALIGN_CENTER_VERTICAL);
    m_waypointChoice = new wxChoice(this, ID_WAYPOINT);
    grid->Add(m_waypointChoice, 1, wxEXPAND);

    top->Add(grid, 0, wxEXPAND | wxALL, 12);
    top->Add(new wxButton(this, ID_REFRESH, "Reload routes"),
             0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    wxFont big = GetFont();
    big.SetPointSize(16);
    big.SetWeight(wxFONTWEIGHT_BOLD);

    auto addValue = [&](const wxString& label, wxStaticText*& value) {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(this, wxID_ANY, label),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        value = new wxStaticText(this, wxID_ANY, "--");
        value->SetFont(big);
        row->Add(value, 1, wxALIGN_CENTER_VERTICAL);
        top->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);
    };

    addValue("Live SOG:", m_sogValue);
    addValue("Remaining distance:", m_distanceValue);
    addValue("Time to go:", m_ttgValue);
    addValue("ETA UTC:", m_etaUtcValue);
    addValue("ETA Local:", m_etaLocalValue);

    m_statusValue = new wxStaticText(this, wxID_ANY, "Waiting for navigation data.");
    top->Add(m_statusValue, 0, wxEXPAND | wxALL, 12);

    SetSizer(top);
    CentreOnParent();

    ReloadRoutes();
    m_timer.Start(1000);
}

void VoyageEtaDialog::ReloadRoutes() {
    m_routeChoice->Clear();
    m_waypointChoice->Clear();
    m_routeGuids.Clear();
    m_points.clear();

    wxArrayString* guids = GetRouteGUIDArray();
    if (!guids || guids->IsEmpty()) {
        m_statusValue->SetLabel("No routes found in OpenCPN.");
        return;
    }

    for (const auto& guid : *guids) {
        std::unique_ptr<PlugIn_Route> route(GetRoute_Plugin(guid));
        if (!route) continue;
        wxString name = route->m_NameString;
        if (name.IsEmpty()) name = guid;
        m_routeChoice->Append(name);
        m_routeGuids.Add(guid);
    }

    if (m_routeChoice->GetCount() > 0) {
        m_routeChoice->SetSelection(0);
        LoadSelectedRoute();
    }
}

bool VoyageEtaDialog::LoadSelectedRoute() {
    m_waypointChoice->Clear();
    m_points.clear();

    int sel = m_routeChoice->GetSelection();
    if (sel == wxNOT_FOUND || sel >= static_cast<int>(m_routeGuids.GetCount()))
        return false;

    std::unique_ptr<PlugIn_Route> route(GetRoute_Plugin(m_routeGuids[sel]));
    if (!route || !route->pWaypointList) return false;

    wxPlugin_WaypointListNode* node = route->pWaypointList->GetFirst();
    size_t number = 1;
    while (node) {
        PlugIn_Waypoint* wp = node->GetData();
        if (wp) {
            RoutePointLite p;
            p.lat = wp->m_lat;
            p.lon = wp->m_lon;
            p.name = wp->m_MarkName;
            if (p.name.IsEmpty()) p.name = wxString::Format("WP %zu", number);
            m_points.push_back(p);
            m_waypointChoice->Append(
                wxString::Format("%zu - %s", number, p.name));
            ++number;
        }
        node = node->GetNext();
    }

    if (!m_points.empty()) {
        m_waypointChoice->SetSelection(
            static_cast<int>(m_points.size() - 1));
        RefreshValues();
        return true;
    }
    return false;
}

double VoyageEtaDialog::DistanceNm(
    double lat1, double lon1, double lat2, double lon2) {
    constexpr double rad = 3.14159265358979323846 / 180.0;
    constexpr double earthNm = 3440.065;
    const double p1 = lat1 * rad;
    const double p2 = lat2 * rad;
    const double dp = (lat2 - lat1) * rad;
    const double dl = (lon2 - lon1) * rad;

    const double a =
        std::sin(dp / 2) * std::sin(dp / 2) +
        std::cos(p1) * std::cos(p2) *
        std::sin(dl / 2) * std::sin(dl / 2);
    const double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return earthNm * c;
}

double VoyageEtaDialog::RemainingDistanceNm(size_t destinationIndex) const {
    if (!m_plugin->HasFix() || m_points.empty() ||
        destinationIndex >= m_points.size())
        return std::numeric_limits<double>::quiet_NaN();

    // Select the nearest route waypoint not beyond the requested destination.
    size_t nearest = 0;
    double nearestDistance = std::numeric_limits<double>::max();
    for (size_t i = 0; i <= destinationIndex; ++i) {
        double d = DistanceNm(
            m_plugin->Latitude(), m_plugin->Longitude(),
            m_points[i].lat, m_points[i].lon);
        if (d < nearestDistance) {
            nearestDistance = d;
            nearest = i;
        }
    }

    double total = nearestDistance;
    for (size_t i = nearest; i < destinationIndex; ++i) {
        total += DistanceNm(
            m_points[i].lat, m_points[i].lon,
            m_points[i + 1].lat, m_points[i + 1].lon);
    }
    return total;
}

wxString VoyageEtaDialog::FormatDuration(double hours) {
    if (!std::isfinite(hours) || hours < 0) return "--";
    long totalMinutes = static_cast<long>(std::llround(hours * 60.0));
    long days = totalMinutes / 1440;
    long hrs = (totalMinutes % 1440) / 60;
    long mins = totalMinutes % 60;
    if (days > 0)
        return wxString::Format("%ld d %02ld h %02ld m", days, hrs, mins);
    return wxString::Format("%02ld h %02ld m", hrs, mins);
}

wxString VoyageEtaDialog::FormatEta(const wxDateTime& dt) {
    return dt.Format("%d %b %Y %H:%M");
}

void VoyageEtaDialog::RefreshValues() {
    const double sog = m_plugin->Sog();
    m_sogValue->SetLabel(wxString::Format("%.1f kn", sog));

    int wpSel = m_waypointChoice->GetSelection();
    if (!m_plugin->HasFix()) {
        m_statusValue->SetLabel("No own-ship GPS fix received from OpenCPN.");
        return;
    }
    if (wpSel == wxNOT_FOUND || m_points.empty()) {
        m_statusValue->SetLabel("Select a route and destination waypoint.");
        return;
    }

    double distance = RemainingDistanceNm(static_cast<size_t>(wpSel));
    if (!std::isfinite(distance)) {
        m_statusValue->SetLabel("Unable to calculate route distance.");
        return;
    }

    m_distanceValue->SetLabel(wxString::Format("%.1f NM", distance));

    if (!std::isfinite(sog) || sog < 0.2) {
        m_ttgValue->SetLabel("--");
        m_etaUtcValue->SetLabel("--");
        m_etaLocalValue->SetLabel("--");
        m_statusValue->SetLabel(
            "SOG below 0.2 kn. ETA calculation paused.");
        return;
    }

    const double hours = distance / sog;
    m_ttgValue->SetLabel(FormatDuration(hours));

    wxDateTime utcNow = wxDateTime::UNow();
    wxTimeSpan span = wxTimeSpan::Minutes(
        static_cast<long>(std::llround(hours * 60.0)));
    wxDateTime etaUtc = utcNow + span;
    wxDateTime etaLocal = etaUtc.ToLocalTime();

    m_etaUtcValue->SetLabel(FormatEta(etaUtc) + " UTC");
    m_etaLocalValue->SetLabel(FormatEta(etaLocal) + " LT");
    m_statusValue->SetLabel(
        "Live ETA based on current SOG and remaining route legs.");
}

void VoyageEtaDialog::OnRouteChanged(wxCommandEvent&) {
    LoadSelectedRoute();
}

void VoyageEtaDialog::OnWaypointChanged(wxCommandEvent&) {
    RefreshValues();
}

void VoyageEtaDialog::OnRefresh(wxCommandEvent&) {
    ReloadRoutes();
}

void VoyageEtaDialog::OnTimer(wxTimerEvent&) {
    RefreshValues();
}

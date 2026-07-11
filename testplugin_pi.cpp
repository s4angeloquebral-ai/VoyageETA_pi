#include "testplugin_pi.h"

#include <wx/artprov.h>
#include <wx/datetime.h>
#include <cmath>
#include <memory>
#include <limits>

#ifndef DECL_EXP
#ifdef __WXMSW__
#define DECL_EXP __declspec(dllexport)
#else
#define DECL_EXP
#endif
#endif

extern "C" DECL_EXP opencpn_plugin* create_pi(void* ppimgr) {
    return new testplugin_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p) {
    delete p;
}

enum {
    ID_ROUTE = wxID_HIGHEST + 500,
    ID_WAYPOINT,
    ID_ACTUAL,
    ID_MANUAL,
    ID_MANUAL_SPEED,
    ID_TIMER
};

wxBEGIN_EVENT_TABLE(VoyageEtaDialog, wxDialog)
    EVT_CHOICE(ID_ROUTE, VoyageEtaDialog::OnRouteChanged)
    EVT_CHOICE(ID_WAYPOINT, VoyageEtaDialog::OnWaypointChanged)
    EVT_RADIOBUTTON(ID_ACTUAL, VoyageEtaDialog::OnSpeedModeChanged)
    EVT_RADIOBUTTON(ID_MANUAL, VoyageEtaDialog::OnSpeedModeChanged)
    EVT_TEXT(ID_MANUAL_SPEED, VoyageEtaDialog::OnManualSpeedChanged)
    EVT_TIMER(ID_TIMER, VoyageEtaDialog::OnTimer)
wxEND_EVENT_TABLE()

testplugin_pi::testplugin_pi(void* ppimgr)
    : opencpn_plugin_118(ppimgr),
      m_icon(wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_TOOLBAR, wxSize(32, 32))) {}

testplugin_pi::~testplugin_pi() = default;

int testplugin_pi::Init() {
    m_toolbarId = InsertPlugInTool(
        "Voyage ETA",
        &m_icon,
        &m_icon,
        wxITEM_NORMAL,
        "Voyage ETA",
        "ETA to destination or waypoint",
        nullptr,
        -1,
        0,
        this
    );

    return WANTS_TOOLBAR_CALLBACK |
           INSTALLS_TOOLBAR_TOOL |
           WANTS_POSITION_FIX_EX;
}

bool testplugin_pi::DeInit() {
    if (m_dialog) {
        m_dialog->Destroy();
        m_dialog = nullptr;
    }
    return true;
}

wxBitmap* testplugin_pi::GetPlugInBitmap() {
    return &m_icon;
}

void testplugin_pi::OnToolbarToolCallback(int) {
    if (!m_dialog) {
        m_dialog = new VoyageEtaDialog(GetOCPNCanvasWindow(), this);
    }

    m_dialog->Show();
    m_dialog->Raise();
    m_dialog->ReloadRoutes();
    m_dialog->RefreshValues();
}

void testplugin_pi::SetPositionFixEx(PlugIn_Position_Fix_Ex& pfix) {
    m_lat = pfix.Lat;
    m_lon = pfix.Lon;
    m_sog = pfix.Sog;
    m_hasFix = true;

    if (m_dialog && m_dialog->IsShown()) {
        m_dialog->RefreshValues();
    }
}

VoyageEtaDialog::VoyageEtaDialog(wxWindow* parent, testplugin_pi* plugin)
    : wxDialog(parent, wxID_ANY, "Voyage ETA",
               wxDefaultPosition, wxSize(500, 560),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_plugin(plugin),
      m_timer(this, ID_TIMER) {

    auto* top = new wxBoxSizer(wxVERTICAL);

    auto* routeGrid = new wxFlexGridSizer(2, 8, 10);
    routeGrid->AddGrowableCol(1, 1);

    routeGrid->Add(new wxStaticText(this, wxID_ANY, "Route:"),
                   0, wxALIGN_CENTER_VERTICAL);
    m_routeChoice = new wxChoice(this, ID_ROUTE);
    routeGrid->Add(m_routeChoice, 1, wxEXPAND);

    routeGrid->Add(new wxStaticText(this, wxID_ANY, "Waypoint:"),
                   0, wxALIGN_CENTER_VERTICAL);
    m_waypointChoice = new wxChoice(this, ID_WAYPOINT);
    routeGrid->Add(m_waypointChoice, 1, wxEXPAND);

    top->Add(routeGrid, 0, wxEXPAND | wxALL, 12);

    auto* speedBox = new wxStaticBoxSizer(wxVERTICAL, this, "Speed");
    m_useActual = new wxRadioButton(this, ID_ACTUAL, "Use actual SOG",
                                    wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_useManual = new wxRadioButton(this, ID_MANUAL, "Use manual speed");

    auto* manualRow = new wxBoxSizer(wxHORIZONTAL);
    manualRow->Add(new wxStaticText(this, wxID_ANY, "Manual speed:"),
                   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    m_manualSpeed = new wxTextCtrl(this, ID_MANUAL_SPEED, "13.0");
    manualRow->Add(m_manualSpeed, 0);
    manualRow->Add(new wxStaticText(this, wxID_ANY, " kn"),
                   0, wxALIGN_CENTER_VERTICAL);

    auto* actualRow = new wxBoxSizer(wxHORIZONTAL);
    actualRow->Add(new wxStaticText(this, wxID_ANY, "Actual SOG:"),
                   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    m_actualSog = new wxStaticText(this, wxID_ANY, "--");
    actualRow->Add(m_actualSog, 0, wxALIGN_CENTER_VERTICAL);

    speedBox->Add(m_useActual, 0, wxALL, 4);
    speedBox->Add(m_useManual, 0, wxALL, 4);
    speedBox->Add(manualRow, 0, wxALL, 4);
    speedBox->Add(actualRow, 0, wxALL, 4);
    top->Add(speedBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    wxFont big = GetFont();
    big.SetPointSize(13);
    big.SetWeight(wxFONTWEIGHT_BOLD);

    auto addResultBox = [&](const wxString& title,
                            wxStaticText*& distance,
                            wxStaticText*& ttg,
                            wxStaticText*& etaUtc,
                            wxStaticText*& etaLocal) {
        auto* box = new wxStaticBoxSizer(wxVERTICAL, this, title);

        auto addRow = [&](const wxString& label, wxStaticText*& value) {
            auto* row = new wxBoxSizer(wxHORIZONTAL);
            row->Add(new wxStaticText(this, wxID_ANY, label),
                     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
            value = new wxStaticText(this, wxID_ANY, "--");
            value->SetFont(big);
            row->Add(value, 1, wxALIGN_CENTER_VERTICAL);
            box->Add(row, 0, wxEXPAND | wxALL, 4);
        };

        addRow("Distance:", distance);
        addRow("TTG:", ttg);
        addRow("ETA UTC:", etaUtc);
        addRow("ETA LT:", etaLocal);

        top->Add(box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    };

    addResultBox("Destination", m_destDistance, m_destTtg,
                 m_destEtaUtc, m_destEtaLocal);

    addResultBox("Selected Waypoint", m_wpDistance, m_wpTtg,
                 m_wpEtaUtc, m_wpEtaLocal);

    m_status = new wxStaticText(this, wxID_ANY, "Waiting for route and position.");
    top->Add(m_status, 0, wxEXPAND | wxALL, 12);

    SetSizer(top);
    CentreOnParent();

    m_useActual->SetValue(true);
    m_manualSpeed->Enable(false);

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
        m_status->SetLabel("No OpenCPN routes found.");
        return;
    }

    for (size_t i = 0; i < guids->GetCount(); ++i) {
        const wxString guid = guids->Item(i);
        std::unique_ptr<PlugIn_Route> route(GetRoute_Plugin(guid));
        if (!route) {
            continue;
        }

        wxString name = route->m_NameString;
        if (name.IsEmpty()) {
            name = guid;
        }

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

    const int sel = m_routeChoice->GetSelection();
    if (sel == wxNOT_FOUND ||
        sel >= static_cast<int>(m_routeGuids.GetCount())) {
        return false;
    }

    std::unique_ptr<PlugIn_Route> route(
        GetRoute_Plugin(m_routeGuids.Item(sel)));

    if (!route || !route->pWaypointList) {
        return false;
    }

    Plugin_WaypointList* list = route->pWaypointList;
    size_t index = 1;

    for (Plugin_WaypointList::iterator it = list->begin();
         it != list->end(); ++it) {
        PlugIn_Waypoint* wp = *it;
        if (!wp) {
            continue;
        }

        VoyageRoutePoint point;
        point.lat = wp->m_lat;
        point.lon = wp->m_lon;
        point.name = wp->m_MarkName;

        if (point.name.IsEmpty()) {
            point.name = wxString::Format("WP %zu", index);
        }

        m_points.push_back(point);
        m_waypointChoice->Append(
            wxString::Format("%zu - %s", index, point.name));

        ++index;
    }

    if (!m_points.empty()) {
        m_waypointChoice->SetSelection(
            static_cast<int>(m_points.size() - 1));
        RefreshValues();
        return true;
    }

    return false;
}

double VoyageEtaDialog::SelectedSpeed() const {
    if (m_useActual->GetValue()) {
        return m_plugin->Sog();
    }

    double speed = 0.0;
    if (!m_manualSpeed->GetValue().ToDouble(&speed)) {
        return 0.0;
    }
    return speed;
}

double VoyageEtaDialog::DistanceNm(
    double lat1, double lon1, double lat2, double lon2) {

    constexpr double pi = 3.14159265358979323846;
    constexpr double earthNm = 3440.065;
    const double rad = pi / 180.0;

    const double p1 = lat1 * rad;
    const double p2 = lat2 * rad;
    const double dp = (lat2 - lat1) * rad;
    const double dl = (lon2 - lon1) * rad;

    const double a =
        std::sin(dp / 2.0) * std::sin(dp / 2.0) +
        std::cos(p1) * std::cos(p2) *
        std::sin(dl / 2.0) * std::sin(dl / 2.0);

    const double c =
        2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));

    return earthNm * c;
}

double VoyageEtaDialog::RemainingDistanceNm(size_t destinationIndex) const {
    if (!m_plugin->HasFix() ||
        m_points.empty() ||
        destinationIndex >= m_points.size()) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    size_t nearest = 0;
    double nearestDistance = std::numeric_limits<double>::max();

    for (size_t i = 0; i <= destinationIndex; ++i) {
        const double d = DistanceNm(
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
    if (!std::isfinite(hours) || hours < 0.0) {
        return "--";
    }

    long minutes = static_cast<long>(std::llround(hours * 60.0));
    const long days = minutes / 1440;
    const long hrs = (minutes % 1440) / 60;
    const long mins = minutes % 60;

    if (days > 0) {
        return wxString::Format("%ld d %02ld h %02ld m", days, hrs, mins);
    }

    return wxString::Format("%02ld h %02ld m", hrs, mins);
}

wxString VoyageEtaDialog::FormatEtaUtc(double hours) {
    if (!std::isfinite(hours) || hours < 0.0) {
        return "--";
    }

    wxDateTime eta = wxDateTime::UNow() +
        wxTimeSpan::Minutes(
            static_cast<long>(std::llround(hours * 60.0)));

    return eta.Format("%d %b %Y %H:%M UTC");
}

wxString VoyageEtaDialog::FormatEtaLocal(double hours) {
    if (!std::isfinite(hours) || hours < 0.0) {
        return "--";
    }

    wxDateTime etaUtc = wxDateTime::UNow() +
        wxTimeSpan::Minutes(
            static_cast<long>(std::llround(hours * 60.0)));

    wxDateTime etaLocal = etaUtc.ToLocalTime();
    return etaLocal.Format("%d %b %Y %H:%M LT");
}

void VoyageEtaDialog::RefreshValues() {
    m_actualSog->SetLabel(
        wxString::Format("%.1f kn", m_plugin->Sog()));

    const double speed = SelectedSpeed();

    if (!m_plugin->HasFix()) {
        m_status->SetLabel("No own-ship position received from OpenCPN.");
        return;
    }

    if (m_points.empty()) {
        m_status->SetLabel("No route waypoints available.");
        return;
    }

    if (!std::isfinite(speed) || speed < 0.2) {
        m_status->SetLabel("Speed must be at least 0.2 kn.");
        return;
    }

    const size_t destIndex = m_points.size() - 1;
    const int wpSelection = m_waypointChoice->GetSelection();

    const double destDistance = RemainingDistanceNm(destIndex);
    const double destHours = destDistance / speed;

    m_destDistance->SetLabel(
        wxString::Format("%.1f NM", destDistance));
    m_destTtg->SetLabel(FormatDuration(destHours));
    m_destEtaUtc->SetLabel(FormatEtaUtc(destHours));
    m_destEtaLocal->SetLabel(FormatEtaLocal(destHours));

    if (wpSelection != wxNOT_FOUND) {
        const size_t wpIndex = static_cast<size_t>(wpSelection);
        const double wpDistance = RemainingDistanceNm(wpIndex);
        const double wpHours = wpDistance / speed;

        m_wpDistance->SetLabel(
            wxString::Format("%.1f NM", wpDistance));
        m_wpTtg->SetLabel(FormatDuration(wpHours));
        m_wpEtaUtc->SetLabel(FormatEtaUtc(wpHours));
        m_wpEtaLocal->SetLabel(FormatEtaLocal(wpHours));
    }

    m_status->SetLabel(
        m_useActual->GetValue()
            ? "Using live actual SOG."
            : "Using manual speed.");
}

void VoyageEtaDialog::OnRouteChanged(wxCommandEvent&) {
    LoadSelectedRoute();
}

void VoyageEtaDialog::OnWaypointChanged(wxCommandEvent&) {
    RefreshValues();
}

void VoyageEtaDialog::OnSpeedModeChanged(wxCommandEvent&) {
    m_manualSpeed->Enable(m_useManual->GetValue());
    RefreshValues();
}

void VoyageEtaDialog::OnManualSpeedChanged(wxCommandEvent&) {
    if (m_useManual->GetValue()) {
        RefreshValues();
    }
}

void VoyageEtaDialog::OnTimer(wxTimerEvent&) {
    RefreshValues();
}

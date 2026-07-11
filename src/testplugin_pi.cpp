#include "testplugin_pi.h"

#include <wx/artprov.h>
#include <wx/datetime.h>

#include <cmath>
#include <limits>
#include <memory>

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
    ID_ROUTE = wxID_HIGHEST + 700,
    ID_WAYPOINT_1,
    ID_WAYPOINT_2,
    ID_WAYPOINT_3,
    ID_ACTUAL,
    ID_MANUAL,
    ID_MANUAL_SPEED,
    ID_TIMER
};

wxBEGIN_EVENT_TABLE(VoyageEtaDialog, wxDialog)
    EVT_CHOICE(ID_ROUTE, VoyageEtaDialog::OnRouteChanged)
    EVT_CHOICE(ID_WAYPOINT_1, VoyageEtaDialog::OnWaypointChanged)
    EVT_CHOICE(ID_WAYPOINT_2, VoyageEtaDialog::OnWaypointChanged)
    EVT_CHOICE(ID_WAYPOINT_3, VoyageEtaDialog::OnWaypointChanged)
    EVT_RADIOBUTTON(ID_ACTUAL, VoyageEtaDialog::OnSpeedModeChanged)
    EVT_RADIOBUTTON(ID_MANUAL, VoyageEtaDialog::OnSpeedModeChanged)
    EVT_TEXT(ID_MANUAL_SPEED, VoyageEtaDialog::OnManualSpeedChanged)
    EVT_TIMER(ID_TIMER, VoyageEtaDialog::OnTimer)
wxEND_EVENT_TABLE()

testplugin_pi::testplugin_pi(void* ppimgr)
    : opencpn_plugin_118(ppimgr),
      m_icon(wxArtProvider::GetBitmap(
          wxART_INFORMATION, wxART_TOOLBAR, wxSize(32, 32))) {}

testplugin_pi::~testplugin_pi() = default;

int testplugin_pi::Init() {
    m_toolbarId = InsertPlugInTool(
        "Voyage ETA",
        &m_icon,
        &m_icon,
        wxITEM_NORMAL,
        "Voyage ETA",
        "ETA to three selected waypoints",
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
    : wxDialog(
          parent,
          wxID_ANY,
          "Voyage ETA",
          wxDefaultPosition,
          wxSize(470, 390),
          wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_plugin(plugin),
      m_timer(this, ID_TIMER) {

    auto* top = new wxBoxSizer(wxVERTICAL);

    auto* routeRow = new wxBoxSizer(wxHORIZONTAL);
    routeRow->Add(
        new wxStaticText(this, wxID_ANY, "Route:"),
        0,
        wxALIGN_CENTER_VERTICAL | wxRIGHT,
        8);
    m_routeChoice = new wxChoice(this, ID_ROUTE);
    routeRow->Add(m_routeChoice, 1, wxEXPAND);
    top->Add(routeRow, 0, wxEXPAND | wxALL, 12);

    auto* speedBox = new wxStaticBoxSizer(wxVERTICAL, this, "Speed");

    m_useActual = new wxRadioButton(
        this, ID_ACTUAL, "Use actual SOG",
        wxDefaultPosition, wxDefaultSize, wxRB_GROUP);

    m_useManual = new wxRadioButton(
        this, ID_MANUAL, "Use manual speed");

    auto* speedRow = new wxBoxSizer(wxHORIZONTAL);
    speedRow->Add(m_useManual, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    m_manualSpeed = new wxTextCtrl(
        this, ID_MANUAL_SPEED, "13.0",
        wxDefaultPosition, wxSize(70, -1));
    speedRow->Add(m_manualSpeed, 0, wxALIGN_CENTER_VERTICAL);
    speedRow->Add(
        new wxStaticText(this, wxID_ANY, " kn"),
        0,
        wxALIGN_CENTER_VERTICAL);

    auto* sogRow = new wxBoxSizer(wxHORIZONTAL);
    sogRow->Add(
        new wxStaticText(this, wxID_ANY, "Actual SOG:"),
        0,
        wxALIGN_CENTER_VERTICAL | wxRIGHT,
        8);
    m_actualSog = new wxStaticText(this, wxID_ANY, "--");
    sogRow->Add(m_actualSog, 0, wxALIGN_CENTER_VERTICAL);

    speedBox->Add(m_useActual, 0, wxALL, 4);
    speedBox->Add(speedRow, 0, wxALL, 4);
    speedBox->Add(sogRow, 0, wxALL, 4);

    top->Add(speedBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    wxFont etaFont = GetFont();
    etaFont.SetPointSize(12);
    etaFont.SetWeight(wxFONTWEIGHT_BOLD);

    auto addWaypointRow = [&](const wxString& title,
                              int id,
                              wxChoice*& choice,
                              wxStaticText*& eta) {
        auto* box = new wxStaticBoxSizer(wxVERTICAL, this, title);

        choice = new wxChoice(this, id);
        box->Add(choice, 0, wxEXPAND | wxALL, 6);

        auto* etaRow = new wxBoxSizer(wxHORIZONTAL);
        etaRow->Add(
            new wxStaticText(this, wxID_ANY, "ETA:"),
            0,
            wxALIGN_CENTER_VERTICAL | wxRIGHT,
            8);

        eta = new wxStaticText(this, wxID_ANY, "--");
        eta->SetFont(etaFont);
        etaRow->Add(eta, 1, wxALIGN_CENTER_VERTICAL);

        box->Add(etaRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
        top->Add(box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    };

    addWaypointRow("Waypoint 1", ID_WAYPOINT_1, m_waypointChoice1, m_eta1);
    addWaypointRow("Waypoint 2", ID_WAYPOINT_2, m_waypointChoice2, m_eta2);
    addWaypointRow("Waypoint 3", ID_WAYPOINT_3, m_waypointChoice3, m_eta3);

    m_status = new wxStaticText(
        this, wxID_ANY, "Waiting for route and position.");
    top->Add(m_status, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    SetSizerAndFit(top);
    CentreOnParent();

    m_useActual->SetValue(true);
    m_manualSpeed->Enable(false);

    ReloadRoutes();
    m_timer.Start(1000);
}

void VoyageEtaDialog::ReloadRoutes() {
    m_routeChoice->Clear();
    m_waypointChoice1->Clear();
    m_waypointChoice2->Clear();
    m_waypointChoice3->Clear();

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
    m_waypointChoice1->Clear();
    m_waypointChoice2->Clear();
    m_waypointChoice3->Clear();
    m_points.clear();

    const int routeSelection = m_routeChoice->GetSelection();

    if (routeSelection == wxNOT_FOUND ||
        routeSelection >= static_cast<int>(m_routeGuids.GetCount())) {
        return false;
    }

    std::unique_ptr<PlugIn_Route> route(
        GetRoute_Plugin(m_routeGuids.Item(routeSelection)));

    if (!route || !route->pWaypointList) {
        return false;
    }

    size_t index = 1;

    for (Plugin_WaypointList::iterator it = route->pWaypointList->begin();
         it != route->pWaypointList->end();
         ++it) {

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

        const wxString label =
            wxString::Format("%zu - %s", index, point.name);

        m_waypointChoice1->Append(label);
        m_waypointChoice2->Append(label);
        m_waypointChoice3->Append(label);

        ++index;
    }

    if (m_points.empty()) {
        m_status->SetLabel("The selected route has no waypoints.");
        return false;
    }

    const int count = static_cast<int>(m_points.size());

    m_waypointChoice1->SetSelection(0);
    m_waypointChoice2->SetSelection(count > 1 ? count / 2 : 0);
    m_waypointChoice3->SetSelection(count - 1);

    RefreshValues();
    return true;
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

    const double phi1 = lat1 * rad;
    const double phi2 = lat2 * rad;
    const double deltaPhi = (lat2 - lat1) * rad;
    const double deltaLambda = (lon2 - lon1) * rad;

    const double a =
        std::sin(deltaPhi / 2.0) * std::sin(deltaPhi / 2.0) +
        std::cos(phi1) * std::cos(phi2) *
        std::sin(deltaLambda / 2.0) * std::sin(deltaLambda / 2.0);

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

    size_t nearestIndex = 0;
    double nearestDistance = std::numeric_limits<double>::max();

    for (size_t i = 0; i <= destinationIndex; ++i) {
        const double distance = DistanceNm(
            m_plugin->Latitude(),
            m_plugin->Longitude(),
            m_points[i].lat,
            m_points[i].lon);

        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearestIndex = i;
        }
    }

    double totalDistance = nearestDistance;

    for (size_t i = nearestIndex; i < destinationIndex; ++i) {
        totalDistance += DistanceNm(
            m_points[i].lat,
            m_points[i].lon,
            m_points[i + 1].lat,
            m_points[i + 1].lon);
    }

    return totalDistance;
}

wxString VoyageEtaDialog::FormatEtaLocal(double hours) {
    if (!std::isfinite(hours) || hours < 0.0) {
        return "--";
    }

    const long minutes =
        static_cast<long>(std::llround(hours * 60.0));

    wxDateTime etaUtc =
        wxDateTime::UNow() + wxTimeSpan::Minutes(minutes);

    wxDateTime etaLocal = etaUtc.ToLocalTime();

    return etaLocal.Format("%d %b %Y %H:%M LT");
}

void VoyageEtaDialog::UpdateEtaForChoice(
    wxChoice* choice,
    wxStaticText* etaLabel) {

    const int selection = choice->GetSelection();
    const double speed = SelectedSpeed();

    if (!m_plugin->HasFix() ||
        selection == wxNOT_FOUND ||
        !std::isfinite(speed) ||
        speed < 0.2) {
        etaLabel->SetLabel("--");
        return;
    }

    const double distance =
        RemainingDistanceNm(static_cast<size_t>(selection));

    if (!std::isfinite(distance)) {
        etaLabel->SetLabel("--");
        return;
    }

    etaLabel->SetLabel(FormatEtaLocal(distance / speed));
}

void VoyageEtaDialog::RefreshValues() {
    m_actualSog->SetLabel(
        wxString::Format("%.1f kn", m_plugin->Sog()));

    UpdateEtaForChoice(m_waypointChoice1, m_eta1);
    UpdateEtaForChoice(m_waypointChoice2, m_eta2);
    UpdateEtaForChoice(m_waypointChoice3, m_eta3);

    if (!m_plugin->HasFix()) {
        m_status->SetLabel("No own-ship position received from OpenCPN.");
        return;
    }

    if (m_points.empty()) {
        m_status->SetLabel("No route waypoints available.");
        return;
    }

    const double speed = SelectedSpeed();

    if (!std::isfinite(speed) || speed < 0.2) {
        m_status->SetLabel("Speed must be at least 0.2 kn.");
        return;
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

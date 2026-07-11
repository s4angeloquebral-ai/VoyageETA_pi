#include "voyage_eta_pi.h"
#include "voyage_eta_dialog.h"
#include <wx/artprov.h>

extern "C" DECL_EXP opencpn_plugin* create_pi(void* ppimgr) {
    return new VoyageEtaPlugin(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p) {
    delete p;
}

VoyageEtaPlugin::VoyageEtaPlugin(void* ppimgr)
    : opencpn_plugin_118(ppimgr) {}

VoyageEtaPlugin::~VoyageEtaPlugin() = default;

int VoyageEtaPlugin::Init() {
    AddLocaleCatalog("opencpn-voyage_eta_pi");

    m_toolbarId = InsertPlugInTool(
        "Voyage ETA",
        wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_TOOLBAR, wxSize(32, 32)),
        wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_TOOLBAR, wxSize(32, 32)),
        wxITEM_NORMAL,
        "Voyage ETA",
        "Live route ETA",
        nullptr,
        -1,
        0,
        this
    );

    return WANTS_TOOLBAR_CALLBACK |
           WANTS_NMEA_EVENTS |
           WANTS_POSITION_FIX_EX;
}

bool VoyageEtaPlugin::DeInit() {
    if (m_dialog) {
        m_dialog->Destroy();
        m_dialog = nullptr;
    }
    return true;
}

wxBitmap* VoyageEtaPlugin::GetPlugInBitmap() {
    static wxBitmap bmp = wxArtProvider::GetBitmap(
        wxART_INFORMATION, wxART_OTHER, wxSize(32, 32));
    return &bmp;
}

void VoyageEtaPlugin::OnToolbarToolCallback(int) {
    if (!m_dialog) {
        m_dialog = new VoyageEtaDialog(GetOCPNCanvasWindow(), this);
    }
    m_dialog->Show();
    m_dialog->Raise();
    m_dialog->ReloadRoutes();
    m_dialog->RefreshValues();
}

void VoyageEtaPlugin::SetPositionFixEx(PlugIn_Position_Fix_Ex& pfix) {
    m_lat = pfix.Lat;
    m_lon = pfix.Lon;
    m_sog = pfix.Sog;
    m_hasFix = true;
    RefreshDialog();
}

void VoyageEtaPlugin::RefreshDialog() {
    if (m_dialog && m_dialog->IsShown()) {
        m_dialog->RefreshValues();
    }
}

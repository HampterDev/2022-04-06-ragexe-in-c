#include "browser/browser_site.h"
#include "browser/browser_address.h"
#include "ui/browser_windows.h"
#include "ui/window_manager_state.h"
#include "render/renderer.h"
#include <docobj.h>
#include <exdispid.h>
#include <mshtmhst.h>
#include <ocidl.h>
#include <stdlib.h>

struct BrowserSite {
    IOleClientSite client;
    IOleInPlaceSite in_place;
    IOleInPlaceFrame frame;
    IDocHostUIHandler host_ui;
    IDispatch dispatch;
    IOleCommandTarget command;
    LONG references;
    HWND hwnd;
    IWebBrowser2 *browser;
    IOleObject *object;
    IConnectionPoint *events;
    DWORD cookie;
    int advised;
    int closing;
};

static ULONG site_add_ref(struct BrowserSite *site)
{
    return (ULONG)InterlockedIncrement(&site->references);
}

static ULONG site_release(struct BrowserSite *site)
{
    ULONG count = (ULONG)InterlockedDecrement(&site->references);
    if (!count) free(site);
    return count;
}

static HRESULT site_query_interface(struct BrowserSite *site, REFIID iid, void **out)
{
    if (!out) return E_POINTER;
    *out = NULL;
    if (!iid) return E_POINTER;
    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IOleClientSite))
        *out = &site->client;
    else if (IsEqualIID(iid, &IID_IOleInPlaceSite) || IsEqualIID(iid, &IID_IOleWindow))
        *out = &site->in_place;
    else if (IsEqualIID(iid, &IID_IOleInPlaceFrame) || IsEqualIID(iid, &IID_IOleInPlaceUIWindow))
        *out = &site->frame;
    else if (IsEqualIID(iid, &IID_IDocHostUIHandler))
        *out = &site->host_ui;
    else if (IsEqualIID(iid, &IID_IDispatch) || IsEqualIID(iid, &DIID_DWebBrowserEvents2))
        *out = &site->dispatch;
    else if (IsEqualIID(iid, &IID_IOleCommandTarget))
        *out = &site->command;
    else return E_NOINTERFACE;
    site_add_ref(site);
    return S_OK;
}

static HRESULT site_get_window(struct BrowserSite *site, HWND *window)
{
    if (!window) return E_POINTER;
    *window = site->hwnd;
    return site->closing ? E_UNEXPECTED : S_OK;
}

static HRESULT STDMETHODCALLTYPE web_client_query_interface(IOleClientSite *self, REFIID riid, void **ppv)
{
    return site_query_interface(CONTAINING_RECORD(self, struct BrowserSite, client), riid, ppv);
}

static ULONG STDMETHODCALLTYPE web_client_add_ref(IOleClientSite *self)
{
    return site_add_ref(CONTAINING_RECORD(self, struct BrowserSite, client));
}

static ULONG STDMETHODCALLTYPE web_client_release(IOleClientSite *self)
{
    return site_release(CONTAINING_RECORD(self, struct BrowserSite, client));
}

static HRESULT STDMETHODCALLTYPE web_client_save_object(IOleClientSite *self)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE web_client_get_moniker(IOleClientSite *self, DWORD assign, DWORD which, IMoniker **moniker)
{
    if (!moniker) return E_POINTER;
    *moniker = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE web_client_get_container(IOleClientSite *self, IOleContainer **container)
{
    if (!container) return E_POINTER;
    *container = NULL;
    return E_NOINTERFACE;
}

static HRESULT STDMETHODCALLTYPE web_client_show_object(IOleClientSite *self)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_client_on_show_window(IOleClientSite *self, BOOL show)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE web_client_request_new_object_layout(IOleClientSite *self)
{
    return E_NOTIMPL;
}

static IOleClientSiteVtbl client_methods = {
    web_client_query_interface,
    web_client_add_ref,
    web_client_release,
    web_client_save_object,
    web_client_get_moniker,
    web_client_get_container,
    web_client_show_object,
    web_client_on_show_window,
    web_client_request_new_object_layout,
};

static HRESULT STDMETHODCALLTYPE web_site_query_interface(IOleInPlaceSite *self, REFIID riid, void **ppv)
{
    return site_query_interface(CONTAINING_RECORD(self, struct BrowserSite, in_place), riid, ppv);
}

static ULONG STDMETHODCALLTYPE web_site_add_ref(IOleInPlaceSite *self)
{
    return site_add_ref(CONTAINING_RECORD(self, struct BrowserSite, in_place));
}

static ULONG STDMETHODCALLTYPE web_site_release(IOleInPlaceSite *self)
{
    return site_release(CONTAINING_RECORD(self, struct BrowserSite, in_place));
}

static HRESULT STDMETHODCALLTYPE web_site_get_window(IOleInPlaceSite *self, HWND *hwnd)
{
    return site_get_window(CONTAINING_RECORD(self, struct BrowserSite, in_place), hwnd);
}

static HRESULT STDMETHODCALLTYPE web_site_context_sensitive_help(IOleInPlaceSite *self, BOOL enter)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_site_can_in_place_activate(IOleInPlaceSite *self)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_site_on_in_place_activate(IOleInPlaceSite *self)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_site_on_ui_activate(IOleInPlaceSite *self)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_site_get_window_context(IOleInPlaceSite *self, IOleInPlaceFrame **frame, IOleInPlaceUIWindow **doc, RECT *pos, RECT *clip, OLEINPLACEFRAMEINFO *info)
{
    struct BrowserSite *site = CONTAINING_RECORD(self, struct BrowserSite, in_place);
    if (!frame || !doc || !pos || !clip || !info) return E_POINTER;
    *frame = NULL;
    *doc = NULL;
    if (site->closing) return E_UNEXPECTED;
    if (!GetClientRect(site->hwnd, pos)) return E_FAIL;
    *clip = *pos;
    info->cb = sizeof *info;
    info->fMDIApp = FALSE;
    info->hwndFrame = site->hwnd;
    info->haccel = NULL;
    info->cAccelEntries = 0;
    *frame = &site->frame;
    site_add_ref(site);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_site_scroll(IOleInPlaceSite *self, SIZE extent)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE web_site_on_ui_deactivate(IOleInPlaceSite *self, BOOL undoable)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_site_on_in_place_deactivate(IOleInPlaceSite *self)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_site_discard_undo_state(IOleInPlaceSite *self)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE web_site_deactivate_and_undo(IOleInPlaceSite *self)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE web_site_on_pos_rect_change(IOleInPlaceSite *self, const RECT *pos)
{
    struct BrowserSite *site = CONTAINING_RECORD(self, struct BrowserSite, in_place);
    IOleInPlaceObject *object = NULL;
    HRESULT hr;
    if (!pos) return E_POINTER;
    if (site->closing || !site->browser) return E_UNEXPECTED;
    hr = site->browser->lpVtbl->QueryInterface(site->browser, &IID_IOleInPlaceObject, (void **)&object);
    if (FAILED(hr)) return hr;
    hr = object->lpVtbl->SetObjectRects(object, pos, pos);
    object->lpVtbl->Release(object);
    return hr;
}

static IOleInPlaceSiteVtbl in_place_methods = {
    web_site_query_interface,
    web_site_add_ref,
    web_site_release,
    web_site_get_window,
    web_site_context_sensitive_help,
    web_site_can_in_place_activate,
    web_site_on_in_place_activate,
    web_site_on_ui_activate,
    web_site_get_window_context,
    web_site_scroll,
    web_site_on_ui_deactivate,
    web_site_on_in_place_deactivate,
    web_site_discard_undo_state,
    web_site_deactivate_and_undo,
    web_site_on_pos_rect_change,
};

static HRESULT STDMETHODCALLTYPE web_frame_query_interface(IOleInPlaceFrame *self, REFIID riid, void **ppv)
{
    return site_query_interface(CONTAINING_RECORD(self, struct BrowserSite, frame), riid, ppv);
}

static ULONG STDMETHODCALLTYPE web_frame_add_ref(IOleInPlaceFrame *self)
{
    return site_add_ref(CONTAINING_RECORD(self, struct BrowserSite, frame));
}

static ULONG STDMETHODCALLTYPE web_frame_release(IOleInPlaceFrame *self)
{
    return site_release(CONTAINING_RECORD(self, struct BrowserSite, frame));
}

static HRESULT STDMETHODCALLTYPE web_frame_get_window(IOleInPlaceFrame *self, HWND *hwnd)
{
    return site_get_window(CONTAINING_RECORD(self, struct BrowserSite, frame), hwnd);
}

static HRESULT STDMETHODCALLTYPE web_frame_context_sensitive_help(IOleInPlaceFrame *self, BOOL enter)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE web_frame_get_border(IOleInPlaceFrame *self, RECT *border)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE web_frame_request_border_space(IOleInPlaceFrame *self, const RECT *widths)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE web_frame_set_border_space(IOleInPlaceFrame *self, const RECT *widths)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE web_frame_set_active_object(IOleInPlaceFrame *self, IOleInPlaceActiveObject *object, LPCOLESTR name)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_frame_insert_menus(IOleInPlaceFrame *self, HMENU menu, LPOLEMENUGROUPWIDTHS widths)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE web_frame_set_menu(IOleInPlaceFrame *self, HMENU menu, HOLEMENU ole_menu, HWND active)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_frame_remove_menus(IOleInPlaceFrame *self, HMENU menu)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE web_frame_set_status_text(IOleInPlaceFrame *self, const WCHAR *text)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_frame_enable_modeless(IOleInPlaceFrame *self, BOOL enable)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_frame_translate_accelerator(IOleInPlaceFrame *self, MSG *msg, WORD id)
{
    return E_NOTIMPL;
}

static IOleInPlaceFrameVtbl frame_methods = {
    web_frame_query_interface,
    web_frame_add_ref,
    web_frame_release,
    web_frame_get_window,
    web_frame_context_sensitive_help,
    web_frame_get_border,
    web_frame_request_border_space,
    web_frame_set_border_space,
    web_frame_set_active_object,
    web_frame_insert_menus,
    web_frame_set_menu,
    web_frame_remove_menus,
    web_frame_set_status_text,
    web_frame_enable_modeless,
    web_frame_translate_accelerator,
};

static HRESULT STDMETHODCALLTYPE web_doc_host_query_interface(IDocHostUIHandler *self, REFIID riid, void **ppv)
{
    return site_query_interface(CONTAINING_RECORD(self, struct BrowserSite, host_ui), riid, ppv);
}

static ULONG STDMETHODCALLTYPE web_doc_host_add_ref(IDocHostUIHandler *self)
{
    return site_add_ref(CONTAINING_RECORD(self, struct BrowserSite, host_ui));
}

static ULONG STDMETHODCALLTYPE web_doc_host_release(IDocHostUIHandler *self)
{
    return site_release(CONTAINING_RECORD(self, struct BrowserSite, host_ui));
}

static HRESULT STDMETHODCALLTYPE web_doc_host_show_context_menu(IDocHostUIHandler *self, DWORD id, POINT *pt, IUnknown *cmd, IDispatch *disp)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_doc_host_get_host_info(IDocHostUIHandler *self, DOCHOSTUIINFO *info)
{
    if (!info) return E_POINTER;
    info->cbSize = sizeof *info;
    info->dwFlags = DOCHOSTUIFLAG_NO3DBORDER;
    info->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
    info->pchHostCss = NULL;
    info->pchHostNS = NULL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_doc_host_show_ui(IDocHostUIHandler *self, DWORD id, IOleInPlaceActiveObject *active, IOleCommandTarget *cmd, IOleInPlaceFrame *frame, IOleInPlaceUIWindow *doc)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_doc_host_hide_ui(IDocHostUIHandler *self)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_doc_host_update_ui(IDocHostUIHandler *self)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_doc_host_enable_modeless(IDocHostUIHandler *self, BOOL enable)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_doc_host_on_doc_window_activate(IDocHostUIHandler *self, BOOL activate)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_doc_host_on_frame_window_activate(IDocHostUIHandler *self, BOOL activate)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_doc_host_resize_border(IDocHostUIHandler *self, const RECT *border, IOleInPlaceUIWindow *window, BOOL frame)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_doc_host_translate_accelerator(IDocHostUIHandler *self, MSG *msg, const GUID *group, DWORD id)
{
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE web_doc_host_get_option_key_path(IDocHostUIHandler *self, WCHAR **key, DWORD reserved)
{
    if (!key) return E_POINTER;
    *key = NULL;
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE web_doc_host_get_drop_target(IDocHostUIHandler *self, IDropTarget *target, IDropTarget **out)
{
    if (!out) return E_POINTER;
    *out = NULL;
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE web_doc_host_get_external(IDocHostUIHandler *self, IDispatch **external)
{
    if (!external) return E_POINTER;
    *external = NULL;
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE web_doc_host_translate_url(IDocHostUIHandler *self, DWORD translate, WCHAR *url_in, WCHAR **url_out)
{
    if (!url_out) return E_POINTER;
    *url_out = NULL;
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE web_doc_host_filter_data_object(IDocHostUIHandler *self, IDataObject *object, IDataObject **out)
{
    if (!out) return E_POINTER;
    *out = NULL;
    return S_FALSE;
}

static IDocHostUIHandlerVtbl host_ui_methods = {
    web_doc_host_query_interface,
    web_doc_host_add_ref,
    web_doc_host_release,
    web_doc_host_show_context_menu,
    web_doc_host_get_host_info,
    web_doc_host_show_ui,
    web_doc_host_hide_ui,
    web_doc_host_update_ui,
    web_doc_host_enable_modeless,
    web_doc_host_on_doc_window_activate,
    web_doc_host_on_frame_window_activate,
    web_doc_host_resize_border,
    web_doc_host_translate_accelerator,
    web_doc_host_get_option_key_path,
    web_doc_host_get_drop_target,
    web_doc_host_get_external,
    web_doc_host_translate_url,
    web_doc_host_filter_data_object,
};

static HRESULT STDMETHODCALLTYPE web_dispatch_query_interface(IDispatch *self, REFIID riid, void **ppv)
{
    return site_query_interface(CONTAINING_RECORD(self, struct BrowserSite, dispatch), riid, ppv);
}

static ULONG STDMETHODCALLTYPE web_dispatch_add_ref(IDispatch *self)
{
    return site_add_ref(CONTAINING_RECORD(self, struct BrowserSite, dispatch));
}

static ULONG STDMETHODCALLTYPE web_dispatch_release(IDispatch *self)
{
    return site_release(CONTAINING_RECORD(self, struct BrowserSite, dispatch));
}

static HRESULT STDMETHODCALLTYPE web_dispatch_get_type_info_count(IDispatch *self, UINT *count)
{
    if (!count) return E_POINTER;
    *count = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE web_dispatch_get_type_info(IDispatch *self, UINT index, LCID lcid, ITypeInfo **info)
{
    if (!info) return E_POINTER;
    *info = NULL;
    return DISP_E_BADINDEX;
}

static HRESULT STDMETHODCALLTYPE web_dispatch_get_ids_of_names(IDispatch *self, REFIID riid, WCHAR **names, UINT count, LCID lcid,
                                                DISPID *ids)
{
    UINT i;
    if (!ids) return E_POINTER;
    for (i = 0; i < count; ++i) ids[i] = DISPID_UNKNOWN;
    return DISP_E_UNKNOWNNAME;
}

static HRESULT STDMETHODCALLTYPE web_dispatch_invoke(IDispatch *self, DISPID id, REFIID riid, LCID lcid, WORD flags,
                                      DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep, UINT *arg_err)
{
    struct BrowserSite *site = CONTAINING_RECORD(self, struct BrowserSite, dispatch);
    struct WebBrowser *wnd;
    char *url;
    if (id != DISPID_NEWWINDOW3) return S_OK;
    if (!params || params->cArgs < 5 || !params->rgvarg) return DISP_E_BADPARAMCOUNT;
    if (params->rgvarg[0].vt != VT_BSTR || params->rgvarg[3].vt != (VT_BYREF | VT_BOOL)
        || !params->rgvarg[3].pboolVal) return DISP_E_TYPEMISMATCH;
    *params->rgvarg[3].pboolVal = VARIANT_TRUE;
    if (site->closing) return S_OK;
    url = browser_address_to_text(params->rgvarg[0].bstrVal, CP_ACP);
    if (!url) return S_OK;
    wnd = ui_window_mgr_new_web_browser(&g_windowMgr, (int)((float)g_renderer->width * 0.7f),
                                   (int)((float)g_renderer->height * 0.7f));
    if (wnd) web_browser_navigate(wnd, url);
    free(url);
    return S_OK;
}

static IDispatchVtbl dispatch_methods = {
    web_dispatch_query_interface,
    web_dispatch_add_ref,
    web_dispatch_release,
    web_dispatch_get_type_info_count,
    web_dispatch_get_type_info,
    web_dispatch_get_ids_of_names,
    web_dispatch_invoke,
};

static HRESULT STDMETHODCALLTYPE web_cmd_query_interface(IOleCommandTarget *self, REFIID riid, void **ppv)
{
    return site_query_interface(CONTAINING_RECORD(self, struct BrowserSite, command), riid, ppv);
}

static ULONG STDMETHODCALLTYPE web_cmd_add_ref(IOleCommandTarget *self)
{
    return site_add_ref(CONTAINING_RECORD(self, struct BrowserSite, command));
}

static ULONG STDMETHODCALLTYPE web_cmd_release(IOleCommandTarget *self)
{
    return site_release(CONTAINING_RECORD(self, struct BrowserSite, command));
}

static HRESULT STDMETHODCALLTYPE web_cmd_query_status(IOleCommandTarget *self, const GUID *group, ULONG count, OLECMD *cmds,
                                       OLECMDTEXT *text)
{
    ULONG i;
    if (count && !cmds) return E_POINTER;
    for (i = 0; i < count; ++i) cmds[i].cmdf = 0;
    return OLECMDERR_E_NOTSUPPORTED;
}

static HRESULT STDMETHODCALLTYPE web_cmd_exec(IOleCommandTarget *self, const GUID *group, DWORD id, DWORD option, VARIANT *in, VARIANT *out)
{
    if (!group || !IsEqualGUID(group, &CGID_DocHostCommandHandler))
        return OLECMDERR_E_UNKNOWNGROUP;
    if (id == 0x28 || id == 0x29)
        return S_OK;
    return OLECMDERR_E_NOTSUPPORTED;
}

static IOleCommandTargetVtbl command_methods = {
    web_cmd_query_interface,
    web_cmd_add_ref,
    web_cmd_release,
    web_cmd_query_status,
    web_cmd_exec,
};

void browser_site_close(struct BrowserSite *site)
{
    IOleObject *object;
    IConnectionPoint *events;
    if (!site || site->closing) return;
    site->closing = 1;
    site->browser = NULL;
    site->hwnd = NULL;
    object = site->object;
    events = site->events;
    site->object = NULL;
    site->events = NULL;
    if (events) {
        if (site->advised) events->lpVtbl->Unadvise(events, site->cookie);
        events->lpVtbl->Release(events);
    }
    if (object) {
        object->lpVtbl->Close(object, OLECLOSE_NOSAVE);
        object->lpVtbl->SetClientSite(object, NULL);
        object->lpVtbl->Release(object);
    }
    site_release(site);
}

HRESULT browser_site_open(HWND hwnd, IWebBrowser2 *browser, struct BrowserSite **out)
{
    struct BrowserSite *site;
    IConnectionPointContainer *container = NULL;
    RECT rect;
    HRESULT hr;
    if (!out) return E_POINTER;
    *out = NULL;
    if (!browser || !hwnd) return E_INVALIDARG;
    site = calloc(1, sizeof *site);
    if (!site) return E_OUTOFMEMORY;
    site->client.lpVtbl = &client_methods;
    site->in_place.lpVtbl = &in_place_methods;
    site->frame.lpVtbl = &frame_methods;
    site->host_ui.lpVtbl = &host_ui_methods;
    site->dispatch.lpVtbl = &dispatch_methods;
    site->command.lpVtbl = &command_methods;
    site->references = 1;
    site->hwnd = hwnd;
    site->browser = browser;
    hr = browser->lpVtbl->QueryInterface(browser, &IID_IOleObject, (void **)&site->object);
    if (FAILED(hr)) goto fail;
    hr = site->object->lpVtbl->SetClientSite(site->object, &site->client);
    if (FAILED(hr)) goto fail;
    hr = OleSetContainedObject((IUnknown *)site->object, TRUE);
    if (FAILED(hr)) goto fail;
    if (!GetClientRect(hwnd, &rect)) { hr = E_FAIL; goto fail; }
    hr = site->object->lpVtbl->DoVerb(site->object, OLEIVERB_INPLACEACTIVATE, NULL, &site->client, 0, hwnd, &rect);
    if (FAILED(hr)) goto fail;
    hr = site->object->lpVtbl->QueryInterface(site->object, &IID_IConnectionPointContainer, (void **)&container);
    if (FAILED(hr)) goto fail;
    hr = container->lpVtbl->FindConnectionPoint(container, &DIID_DWebBrowserEvents2, &site->events);
    container->lpVtbl->Release(container);
    if (FAILED(hr)) goto fail;
    hr = site->events->lpVtbl->Advise(site->events, (IUnknown *)&site->dispatch, &site->cookie);
    if (FAILED(hr)) goto fail;
    site->advised = 1;
    *out = site;
    return S_OK;
fail:
    browser_site_close(site);
    return hr;
}
